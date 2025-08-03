#include "KV_Storage.h"
#include "zf_common_headfile.h"

int32 kv_storage_cache[512*48];

KeyValuePair KV_List[MAX_KV_PAIRS] = {0};

void kv_storage_init(void) {
    // 初始化 KV 存储，可以在这里加载持久化存储的数据
    flash_init(); // 确保 flash 已经初始化
    flash_buffer_clear(); // 清空数据缓冲区
    memset(kv_storage_cache, 0, sizeof(kv_storage_cache));
}

bool kv_storage_set_float(const char *key, float value){
    // 计算存储的页索引
    for (int i = 0; i < MAX_KV_PAIRS; i++) {
        if (strcmp(KV_List[i].key, key) == 0) {
            // 如果键已存在，更新值
            KV_List[i].is_float = true;
            KV_List[i].float_value = value;
            return true;
        }
    }
    // 如果键不存在，找到一个空位存储
    for (int i = 0; i < MAX_KV_PAIRS; i++) {
        if (KV_List[i].key[0] == '\0') {
            // 找到一个空位，存储新的键值对
            strncpy(KV_List[i].key, key, sizeof(KV_List[i].key) - 1);
            KV_List[i].float_value = value;
            KV_List[i].is_float = true;
            return true;
        }
    }
    printf("Unable to store float value: %s\n", key);
    return false; // 存储失败
}

bool kv_storage_set_int(const char *key, int value) {
    // 计算存储的页索引
    for (int i = 0; i < MAX_KV_PAIRS; i++) {
        if (strcmp(KV_List[i].key, key) == 0) {
            // 如果键已存在，更新值
            KV_List[i].is_float = false;
            KV_List[i].int_value = value;
            return true;
        }
    }
    // 如果键不存在，找到一个空位存储
    for (int i = 0; i < MAX_KV_PAIRS; i++) {
        if (KV_List[i].key[0] == '\0') {
            // 找到一个空位，存储新的键值对
            strncpy(KV_List[i].key, key, sizeof(KV_List[i].key) - 1);
            KV_List[i].int_value = value;
            KV_List[i].is_float = false;
            return true;
        }
    }
    printf("Unable to store int value: %s\n", key);
    return false; // 存储失败
}


bool kv_storage_get_int(const char *key, int *value) {
    for (int i = 0; i < MAX_KV_PAIRS; i++) {
        if (strcmp(KV_List[i].key, key) == 0) {
            // 找到键，返回对应的值
            if (!KV_List[i].is_float) {
                *value = KV_List[i].int_value;
                return true;
            }
        }
    }
    printf("Key not found: %s\n", key);
    return false; // 未找到键
}

bool kv_storage_get_float(const char *key, float *value) {
    for (int i = 0; i < MAX_KV_PAIRS; i++) {
        if (strcmp(KV_List[i].key, key) == 0) {
            // 找到键，返回对应的值
            if (KV_List[i].is_float) {
                *value = KV_List[i].float_value;
                return true;
            }
        }
    }
    printf("Key not found: %s\n", key);
    return false; // 未找到键
}

bool kv_storage_remove(const char *key) {
    for (int i = 0; i < MAX_KV_PAIRS; i++) {
        if (strcmp(KV_List[i].key, key) == 0) {
            // 找到键，删除对应的值
            memset(&KV_List[i], 0, sizeof(KeyValuePair));
            return true;
        }
    }
    printf("Key not found for removal: %s\n", key);
    return false; // 未找到键
}

bool kv_storage_read_from_flash(void) {
    // 从 flash 中读取数据到 KV_Storage_Cache
    for (int i = 0; i < 48; i++) {
        flash_read_page_to_buffer(0, i, FLASH_PAGE_LENGTH);
        // 将数据拷贝到 kv_storage_cache
        for (int j = 0; j < 512; j++) {
            kv_storage_cache[i * 512 + j] = flash_union_buffer[j].int32_type;
        }
    }
    return true;
}

bool kv_storage_write_to_flash(void) {
    // 首先，将 KV_List 的当前状态更新到缓存中
    kv_update_cache_from_kv_list();

    // 将 KV_Storage_Cache 中的数据写入 flash
    for (int i = 0; i < 48; i++) {
        for (int j = 0; j < 512; j++) {
            flash_union_buffer[j].int32_type = kv_storage_cache[i * 512 + j];
        }
        flash_write_page_from_buffer(0, i, FLASH_PAGE_LENGTH);
    }
    return true;
}

bool kv_parse_data_to_kv_list(void) {
    // 确保数据大小不会溢出
    if (sizeof(KV_List) > sizeof(kv_storage_cache)) {
        // 处理错误，可能记录日志或返回 false
        return false;
    }
    // 从缓存中加载数据到 KV_List
    memcpy(KV_List, kv_storage_cache, sizeof(KV_List));
    return true;
}

bool kv_update_cache_from_kv_list(void) {
    // 确保数据大小不会溢出
    if (sizeof(KV_List) > sizeof(kv_storage_cache)) {
        // 处理错误
        return false;
    }
    // 将 KV_List 的数据拷贝到缓存
    memcpy(kv_storage_cache, KV_List, sizeof(KV_List));
    return true;
}
    
void kv_load_data(void) {
    // 从 flash 中加载数据到 KV_List
    kv_storage_read_from_flash();
    kv_parse_data_to_kv_list();
}

void kv_save_data(void) {
    // 将 KV_List 的数据保存到 flash
    kv_update_cache_from_kv_list();
    kv_storage_write_to_flash();
}

bool kv_storage_format(void) {
    // 清空 KV_List
    memset(KV_List, 0, sizeof(KV_List));
    // 清空缓存
    memset(kv_storage_cache, 0, sizeof(kv_storage_cache));
    // 清空 flash 中的数据
    for (int i = 0; i < 48; i++) {
        flash_erase_page(0, i);
    }
    printf("KV Storage formatted.\n");
    return true;
}