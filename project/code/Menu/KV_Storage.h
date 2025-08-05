#ifndef KV_STORAGE_H
#define KV_STORAGE_H

#include "zf_common_typedef.h"


#define MAX_KV_PAIRS 64 // 最大键值对数量，请勿超过Flash页的存储能力

typedef struct {
    char key[59]; // 键的最大长度为 54 字符 + 1 字符结尾
    int32 int_value; // 存储整数值
    float float_value; // 存储浮点值
    int8 is_float; // 标记是否有浮点值
} KeyValuePair;
// KeyValuePair的大小是 sizeof(char[59]) + sizeof(flash_data_union) + sizeof(int8)，即 59 + 4 + 1 = 64 字节，一页可以存储 512 / 64 = 8 个键值对


#include "zf_common_headfile.h"
#include <stdbool.h> // 使用标准的 bool 类型

extern KeyValuePair KV_List[MAX_KV_PAIRS];

void kv_load_data(void);
void kv_save_data(void);
void kv_storage_init(void);
bool kv_storage_set_float(const char *key, float value);
bool kv_storage_set_int(const char *key, int value);
bool kv_storage_get_int(const char *key, int *value);
bool kv_storage_get_float(const char *key, float *value);
bool kv_storage_remove(const char *key);
bool kv_storage_format(void);
bool kv_update_cache_from_kv_list(void);
void kv_cli(void);
#endif // KV_STORAGE_H
