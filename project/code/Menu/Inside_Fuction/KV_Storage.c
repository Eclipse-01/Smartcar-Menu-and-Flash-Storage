#include "KV_Storage.h"
#include "zf_common_headfile.h"
#include <string.h> // 用于 memcpy 和 memset

/*
 * 优化说明:
 * 1. 移除了全局缓存数组 `kv_storage_cache[512*48]`，这是主要的内存优化点，节省了96KB的RAM。
 * 2. 重写了 `kv_storage_read_from_flash` 和 `kv_storage_write_to_flash` 函数。
 * 现在它们直接在 `KV_List` 数组和Flash的底层IO缓冲区之间逐页传输数据，避免了大的中间缓存。
 * 3. 移除了 `kv_parse_data_to_kv_list` 和 `kv_update_cache_from_kv_list`，因为它们只为已删除的缓存服务。
 * 4. 优化了 `set` 和 `get` 函数的内部逻辑，使用静态辅助函数来减少代码重复，并使逻辑更清晰。
 * 5. `kv_storage_format` 现在会根据 `KV_List` 的实际大小计算需要擦除的Flash页数，更具灵活性。
 * 6. 所有对外接口（函数调用方式）保持不变。
 */


// 全局键值对列表，这是数据在内存中的主要形式
KeyValuePair KV_List[MAX_KV_PAIRS] = {0};

// 为清晰起见，定义Flash页的字节大小 (假设 flash_union_buffer 是512个int32)
#define FLASH_PAGE_SIZE_BYTES (512 * sizeof(int32))


// --- 内部辅助函数 ---

/**
 * @brief 查找指定key在KV_List中的索引
 * @param key 要查找的键
 * @return 如果找到，返回索引；否则返回-1
 */
static int find_key_index(const char *key) {
    for (int i = 0; i < MAX_KV_PAIRS; i++) {
        // 检查槽位是否被使用且key匹配
        if (KV_List[i].key[0] != '\0' && strcmp(KV_List[i].key, key) == 0) {
            return i;
        }
    }
    return -1; // 未找到
}

/**
 * @brief 在KV_List中查找一个空闲的槽位
 * @return 如果找到，返回索引；否则返回-1
 */
static int find_empty_slot(void) {
    for (int i = 0; i < MAX_KV_PAIRS; i++) {
        if (KV_List[i].key[0] == '\0') {
            return i;
        }
    }
    return -1; // 没有空闲槽位
}


// --- 公共API函数 (接口保持不变) ---

void kv_storage_init(void) {
    // 初始化KV存储
    flash_init(); // 确保flash已初始化
    // 在从flash加载数据前，清空内存中的列表
    memset(KV_List, 0, sizeof(KV_List));
}

bool kv_storage_set_float(const char *key, float value) {
    int index = find_key_index(key);
    if (index == -1) { // 键不存在，查找新槽位
        index = find_empty_slot();
        if (index == -1) {
            printf("KV存储已满，无法存储浮点值: %s\n", key);
            return false; // 存储已满
        }
        // 将新键复制到槽位
        strncpy(KV_List[index].key, key, sizeof(KV_List[index].key) - 1);
        KV_List[index].key[sizeof(KV_List[index].key) - 1] = '\0'; // 确保字符串以null结尾
    }

    // 设置值
    KV_List[index].is_float = true;
    KV_List[index].float_value = value;
    return true;
}

bool kv_storage_set_int(const char *key, int value) {
    int index = find_key_index(key);
    if (index == -1) { // 键不存在，查找新槽位
        index = find_empty_slot();
        if (index == -1) {
            printf("KV存储已满，无法存储整数值: %s\n", key);
            return false; // 存储已满
        }
        // 将新键复制到槽位
        strncpy(KV_List[index].key, key, sizeof(KV_List[index].key) - 1);
        KV_List[index].key[sizeof(KV_List[index].key) - 1] = '\0'; // 确保字符串以null结尾
    }

    // 设置值
    KV_List[index].is_float = false;
    KV_List[index].int_value = value;
    return true;
}

bool kv_storage_get_int(const char *key, int *value) {
    int index = find_key_index(key);
    if (index != -1 && !KV_List[index].is_float) {
        *value = KV_List[index].int_value;
        return true;
    }
    printf("未找到整数类型的键或类型不匹配: %s\n", key);
    return false; // 未找到键或类型不匹配
}

bool kv_storage_get_float(const char *key, float *value) {
    int index = find_key_index(key);
    if (index != -1 && KV_List[index].is_float) {
        *value = KV_List[index].float_value;
        return true;
    }
    printf("未找到浮点类型的键或类型不匹配: %s\n", key);
    return false; // 未找到键或类型不匹配
}

bool kv_storage_remove(const char *key) {
    int index = find_key_index(key);
    if (index != -1) {
        // 找到键，清空该条目
        memset(&KV_List[index], 0, sizeof(KeyValuePair));
        return true;
    }
    printf("未找到要删除的键: %s\n", key);
    return false;
}


// --- 优化后的Flash读写函数 ---

bool kv_storage_read_from_flash(void) {
    // 直接从Flash逐页读取数据到KV_List数组，无需大的中间缓存
    uint8_t *kv_ptr = (uint8_t *)KV_List;
    size_t remaining_bytes = sizeof(KV_List);
    int page_index = 0;

    while (remaining_bytes > 0) {
        // 从Flash读取一页数据到硬件IO缓冲区 (flash_union_buffer)
        // 假设 flash_read_page_to_buffer 使用了全局的 flash_union_buffer
        flash_read_page_to_buffer(0, page_index, FLASH_PAGE_LENGTH);

        // 决定本次迭代要复制多少字节
        size_t bytes_to_copy = (remaining_bytes < FLASH_PAGE_SIZE_BYTES) ? remaining_bytes : FLASH_PAGE_SIZE_BYTES;

        // 从Flash缓冲区直接复制到KV_List的对应位置
        memcpy(kv_ptr, flash_union_buffer, bytes_to_copy);

        // 更新指针和计数器
        kv_ptr += bytes_to_copy;
        remaining_bytes -= bytes_to_copy;
        page_index++;
    }

    return true;
}

bool kv_storage_write_to_flash(void) {
    // 直接从KV_List数组逐页写入数据到Flash
    const uint8_t *kv_ptr = (const uint8_t *)KV_List;
    size_t remaining_bytes = sizeof(KV_List);
    int page_index = 0;

    // 注意：一个健壮的系统在写入前需要确保Flash已被擦除。
    // 此处遵循原逻辑，假设擦除操作由 format 函数负责。

    while (remaining_bytes > 0) {
        // 决定本次迭代要复制多少字节
        size_t bytes_to_copy = (remaining_bytes < FLASH_PAGE_SIZE_BYTES) ? remaining_bytes : FLASH_PAGE_SIZE_BYTES;

        // 如果是最后一页且数据不足一页，用擦除状态值(通常是0xFF)填充缓冲区的剩余部分
        if (bytes_to_copy < FLASH_PAGE_SIZE_BYTES) {
            memset((uint8_t*)flash_union_buffer + bytes_to_copy, 0xFF, FLASH_PAGE_SIZE_BYTES - bytes_to_copy);
        }

        // 从KV_List复制一块数据到Flash缓冲区
        memcpy(flash_union_buffer, kv_ptr, bytes_to_copy);

        // 将缓冲区的数据写入Flash的一页
        flash_write_page_from_buffer(0, page_index, FLASH_PAGE_LENGTH);

        // 更新指针和计数器
        kv_ptr += bytes_to_copy;
        remaining_bytes -= bytes_to_copy;
        page_index++;
    }

    return true;
}


// --- 上层封装函数 ---

void kv_load_data(void) {
    // 简化：直接从Flash读取到KV_List
    kv_storage_read_from_flash();
}

void kv_save_data(void) {
    // 简化：直接从KV_List写入到Flash
    kv_storage_write_to_flash();
}

bool kv_storage_format(void) {
    // 清空内存中的列表
    memset(KV_List, 0, sizeof(KV_List));

    // 根据KV_List的实际大小计算需要擦除的页数
    int num_pages = (sizeof(KV_List) + FLASH_PAGE_SIZE_BYTES - 1) / FLASH_PAGE_SIZE_BYTES;
    
    // 擦除Flash中对应的页
    for (int i = 0; i < num_pages; i++) {
        flash_erase_page(0, i);
    }

    printf("KV存储已格式化。\n");
    return true;
}
