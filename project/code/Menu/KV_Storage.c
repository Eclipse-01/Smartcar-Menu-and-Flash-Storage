#include "KV_Storage.h"
#include "zf_common_headfile.h"
#include <string.h> // 用于 memcpy 和 memset

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
            printf("Unable to save key %s due to no avaliable space.\n", key);
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
            printf("Unable to save key %s due to no aavaliable space.\n", key);
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
    printf("Cannot find interger with correspond key: %s\n", key);
    return false; // 未找到键或类型不匹配
}

bool kv_storage_get_float(const char *key, float *value) {
    int index = find_key_index(key);
    if (index != -1 && KV_List[index].is_float) {
        *value = KV_List[index].float_value;
        return true;
    }
    printf("Cannot find float with correspond key: %s\n", key);
    return false; // 未找到键或类型不匹配
}

bool kv_storage_remove(const char *key) {
    int index = find_key_index(key);
    if (index != -1) {
        // 找到键，清空该条目
        memset(&KV_List[index], 0, sizeof(KeyValuePair));
        return true;
    }
    printf("Key %s does not exist. Can not delete.\n", key);
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
        memset(flash_union_buffer, 0xFF, FLASH_PAGE_SIZE_BYTES); // 将缓冲区填充为0xFF
        flash_write_page_from_buffer(0, i, FLASH_PAGE_LENGTH); // 写入缓冲区到Flash
    }
    flash_buffer_clear(); // 清除缓冲区状态
    kv_save_data(); // 保存格式化结果到Flash
    kv_load_data(); // 重新加载数据以确保内存中的KV_List是空的
    kv_storage_set_int("Format Result", 1); // 设置格式化状态键值对
    kv_save_data(); // 保存格式化结果到Flash
    // 只有这样才不会出错。
    return true;
}










// 下面的函数是CLI命令的实现
#define UART_INDEX              (DEBUG_UART_INDEX   )                           // 默认 UART_0
#define UART_BAUDRATE           (DEBUG_UART_BAUDRATE)                           // 默认 115200
#define UART_TX_PIN             (DEBUG_UART_TX_PIN  )                           // 默认 UART0_TX_P00_1
#define UART_RX_PIN             (DEBUG_UART_RX_PIN  )                           // 默认 UART0_RX_P00_0

uint8 uart_get_data[64];                                                        // 串口接收数据缓冲区
uint8 fifo_get_data[64];                                                        // fifo 输出读出缓冲区

uint8  get_data = 0;                                                            // 接收数据变量
uint32 fifo_data_count = 0;                                                     // fifo 数据个数

fifo_struct uart_data_fifo;

// Flash KV工具相关变量
char command_buffer[128];                                                       // 命令缓冲区
uint8 command_index = 0;                                                        // 命令索引
bool command_ready = false;                                                     // 命令是否准备就绪

// 函数声明
void process_command(char* cmd);
void show_all_kv_pairs(void);
void find_key(char* key);
void delete_key(char* key);
void format_flash(void);
void show_help(void);
void parse_set_command(char* cmd);
void show_status(void);
void show_version(void);
void clear_screen(void);
char* complete_command(char* partial);

void kv_cli(void)
{
    fifo_init(&uart_data_fifo, FIFO_DATA_8BIT, uart_get_data, 64);              // 初始化 fifo 挂载缓冲区
    uart_init(UART_INDEX, UART_BAUDRATE, UART_TX_PIN, UART_RX_PIN);             // 初始化串口
    uart_rx_interrupt(UART_INDEX, 1);                                           // 开启 UART_INDEX 的接收中断                                          
    kv_storage_init();                                                      // 格式化 KV 存储                                                   // 格式化 Flash 存储
    kv_load_data();
    timer_init(TC_TIME2_CH0, TIMER_MS);                                    // 初始化计时器 TC_TIME2_CH0 为毫秒计时模式
    timer_start(TC_TIME2_CH0);                                       // 启动计时器 TC_TIME2_CH0
    // 显示欢迎信息和帮助
    uart_write_string(UART_INDEX, "\r\n\033[1;36m===========================================\033[0m\r\n");
    uart_write_string(UART_INDEX, "\033[1;36m    Flash KV Storage Manage CLI v2.0\033[0m\r\n");
    uart_write_string(UART_INDEX, "\033[1;36m===========================================\033[0m\r\n");
    uart_write_string(UART_INDEX, "Type '\033[1;33mhelp\033[0m' to see available commands\r\n");
    show_help();
    uart_write_string(UART_INDEX, "\r\n\033[1;32mKV>\033[0m ");
    
    // 初始化命令缓冲区
    memset(command_buffer, 0, sizeof(command_buffer));
    command_index = 0;
    command_ready = false;

    bool exit_cli = false; // 新增退出标志
    
    // 此处编写用户代码 例如外设初始化代码等
    for(;;)
    {
        if(exit_cli) break; // 检查退出标志

        // 此处编写需要循环执行的代码
        
        fifo_data_count = fifo_used(&uart_data_fifo);                           // 查看 fifo 是否有数据
        if(fifo_data_count != 0)                                                // 读取到数据了
        {
            fifo_read_buffer(&uart_data_fifo, fifo_get_data, &fifo_data_count, FIFO_READ_AND_CLEAN);    // 将 fifo 中数据读出并清空 fifo 挂载的缓冲
            
            // 处理接收到的数据，构建命令
            for(uint32 i = 0; i < fifo_data_count; i++)
            {
                char c = (char)fifo_get_data[i];
                
                if(c == '\r' || c == '\n')  // 回车或换行表示命令结束
                {
                    if(command_index > 0)
                    {
                        command_buffer[command_index] = '\0';
                        command_ready = true;
                        break;
                    }
                }
                else if(c == '\b' || c == 127)  // 退格处理
                {
                    if(command_index > 0)
                    {
                        command_index--;
                        uart_write_string(UART_INDEX, "\b \b");  // 回显退格
                    }
                }
                else if(command_index < sizeof(command_buffer) - 1)
                {
                    command_buffer[command_index++] = c;
                    uart_write_byte(UART_INDEX, c);  // 回显字符
                }
            }
        }
        
        // 处理完整的命令
        if(command_ready)
        {
            uart_write_string(UART_INDEX, "\r\n");
            // 检查是否为退出命令
            if(strncmp(command_buffer, "exit", 4) == 0)
            {
                uart_write_string(UART_INDEX, "Exiting CLI...\r\n");
                exit_cli = true;
            }
            else
            {
                process_command(command_buffer);
                uart_write_string(UART_INDEX, "\r\n\033[1;32mKV>\033[0m ");
            }
            
            // 重置命令缓冲区
            memset(command_buffer, 0, sizeof(command_buffer));
            command_index = 0;
            command_ready = false;
        }
        
        system_delay_ms(10);
      
      
        // 此处编写需要循环执行的代码
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       UART_INDEX 的接收中断处理函数 这个函数将在 UART_INDEX 对应的中断调用
// 参数说明       void
// 返回参数       void
// 使用示例       uart_rx_interrupt_handler();
//-------------------------------------------------------------------------------------------------------------------
void uart_rx_interrupt_handler (void)
{
//    get_data = uart_read_byte(UART_INDEX);                                      // 接收数据 while 等待式 不建议在中断使用
    if(uart_query_byte(UART_INDEX, &get_data))                                  // 接收数据 查询式 有数据会返回 TRUE 没有数据会返回 FALSE
    {
        fifo_write_buffer(&uart_data_fifo, &get_data, 1);                       // 将数据写入 fifo 中
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       显示帮助信息
// 参数说明       void
// 返回参数       void
//-------------------------------------------------------------------------------------------------------------------
void show_help(void)
{
    uart_write_string(UART_INDEX, "\r\n\033[1;36m========== KV Storage CLI Commands ==========\033[0m\r\n");
    uart_write_string(UART_INDEX, "\033[1;32mData Management:\033[0m\r\n");
    uart_write_string(UART_INDEX, "  \033[1;33mlist\033[0m (ls)      - Show all key-value pairs\r\n");
    uart_write_string(UART_INDEX, "  \033[1;33mfind\033[0m <key>    - Find a specific key\r\n");
    uart_write_string(UART_INDEX, "  \033[1;33mset\033[0m <key> <val> - Set a key-value pair (auto-detect type)\r\n");
    uart_write_string(UART_INDEX, "  \033[1;33mdelete\033[0m (rm) <key> - Delete a specific key-value pair\r\n");
    uart_write_string(UART_INDEX, "\r\n\033[1;32mStorage Operations:\033[0m\r\n");
    uart_write_string(UART_INDEX, "  \033[1;33msave\033[0m         - Save data to Flash\r\n");
    uart_write_string(UART_INDEX, "  \033[1;33mload\033[0m         - Load data from Flash\r\n");
    uart_write_string(UART_INDEX, "  \033[1;33mformat\033[0m       - Format Flash storage\r\n");
    uart_write_string(UART_INDEX, "\r\n\033[1;32mUtility Commands:\033[0m\r\n");
    uart_write_string(UART_INDEX, "  \033[1;33mstatus\033[0m       - Show storage statistics\r\n");
    uart_write_string(UART_INDEX, "  \033[1;33mclear\033[0m        - Clear screen\r\n");
    uart_write_string(UART_INDEX, "  \033[1;33mversion\033[0m      - Show version information\r\n");
    uart_write_string(UART_INDEX, "  \033[1;33mhelp\033[0m (h, ?)  - Show this help information\r\n");
    uart_write_string(UART_INDEX, "  \033[1;33mexit\033[0m (quit)  - Exit the CLI\r\n");
    uart_write_string(UART_INDEX, "\r\n\033[1;34mTips:\033[0m\r\n");
    uart_write_string(UART_INDEX, "  • Use TAB for command completion\r\n");
    uart_write_string(UART_INDEX, "  • Aliases are shown in parentheses\r\n");
    uart_write_string(UART_INDEX, "  • First time? Run 'format' then 'status'\r\n");
    uart_write_string(UART_INDEX, "  • Remember to 'save' after making changes\r\n");
    uart_write_string(UART_INDEX, "\r\n\033[1;35mSupport:\033[0m https://github.com/Eclipse-01/\r\n");
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       处理串口命令
// 参数说明       cmd - 命令字符串
// 返回参数       void
//-------------------------------------------------------------------------------------------------------------------
void process_command(char* cmd)
{
    while(*cmd == ' ') cmd++;

    if(strlen(cmd) == 0) return;

    // Main commands and aliases
    if(strncmp(cmd, "list", 4) == 0 || strncmp(cmd, "ls", 2) == 0)
    {
        show_all_kv_pairs();
    }
    else if(strncmp(cmd, "find ", 5) == 0)
    {
        find_key(cmd + 5);
    }
    else if(strncmp(cmd, "delete ", 7) == 0 || strncmp(cmd, "rm ", 3) == 0)
    {
        if(strncmp(cmd, "delete ", 7) == 0)
            delete_key(cmd + 7);
        else
            delete_key(cmd + 3);
    }
    else if(strncmp(cmd, "set ", 4) == 0)
    {
        parse_set_command(cmd + 4);
    }
    else if(strncmp(cmd, "format", 6) == 0)
    {
        format_flash();
    }
    else if(strncmp(cmd, "save", 4) == 0)
    {
        uart_write_string(UART_INDEX, "\033[1;33mSaving data to Flash...\033[0m\r\n");
        kv_save_data();
        uart_write_string(UART_INDEX, "\033[1;32mData saved to Flash successfully\033[0m\r\n");
    }
    else if(strncmp(cmd, "load", 4) == 0)
    {
        kv_load_data();
        uart_write_string(UART_INDEX, "\033[1;32mData loaded from Flash successfully\033[0m\r\n");
    }
    else if(strncmp(cmd, "help", 4) == 0 || strcmp(cmd, "h") == 0 || strcmp(cmd, "?") == 0)
    {
        show_help();
    }
    else if(strncmp(cmd, "status", 6) == 0)
    {
        show_status();
    }
    else if(strncmp(cmd, "version", 7) == 0)
    {
        show_version();
    }
    else if(strncmp(cmd, "clear", 5) == 0)
    {
        clear_screen();
    }
    else if(strncmp(cmd, "exit", 4) == 0 || strncmp(cmd, "quit", 4) == 0)
    {
        uart_write_string(UART_INDEX, "\033[1;36mExiting CLI...\033[0m\r\n");
        return;
    }
    else if(strcmp(cmd, "haavk") == 0 || strcmp(cmd, "HAAVK") == 0)
    {
        const char *Haavk_Strings[] = {
            "天空属于哈夫克", 
            "战争让烬区千疮百孔…… \n哈夫克渴望赋予其全新面貌，但G.T.I显然想让硝烟在此地永不熄灭，他们宣称要重新定义这片荒漠的秩序。\n无需冒进，先借助哨卡给敌人当头一击！随后利用公路两侧建筑和防御工事拖住他们，坦克部队将穿梭阵地碾碎敌人的野心！\n真理只在导弹射程之内……",
            "天网升级后，天空属于哈夫克将不再是一句空话！\n\n  但这就是所有的奇迹了吗？\n\n——不，他只是冰山一角。\n\n  天网、Relink、曼德尔砖、诺翁芯片、航天工程……他们都是我们绘制新世界的画笔！他们将加速哈夫克改写人类未来的进程！\n\n  而战争……战争是外交辞令，是沟通法宝，它将确保我们的蓝图得到精准绘制，确保哈夫克建立资源平等型社会的终极愿景得以实现。\n",
            "你们不要再祈求回到从前，我们的钟表可以回到原点，但我们永远不可能回到昨天了。你们也不要再来问我战争何时结束，哈夫克的日常里没有这个字眼。\n\n  从航天城开幕式的恐袭到巴别塔Relink发布会上的炸弹，人们不愿意相信理想者的善意，人们只相信子弹才是真理！\n\n  我们别无选择……我们不止北伐或南征，要战遍世界！让全球彻底听懂，哈夫克所要创建的伟业！",
            "若太阳熄灭，哈夫克将保证翌日的黎明照常升起。\n\n  哈夫克电台里刚响起典狱长气势磅礴的宣言，他们便被一位潮汐监狱的囚犯卷进了全球风暴。\n\n  G.T.I则从未想到：他们会在一处洲际导弹基地遭遇曾在他们口中被打击了无数次的气象武器。\n\n  双方指挥官在沙盘上的每次推演都获得了凯旋。但当战争开始时，双方的每次行动都失败了。\n\n  是的，与战争共处太久后，就连胜利者也会被胜利打败。\n",
            "洲际导弹基地关乎着天网武器的成败，是加大区域威慑半径确保天空属于哈夫克的利剑；是我们实现资源平等型社会、打造新世界的加速器！\n\n  但现在基地坐标被渡鸦这个丧心病狂、忘恩负义的疯子泄露给了G.T.I！ 既然他们想让这里成为新的风暴中心，那就如他们所愿！\n\n  波塞冬系列导弹会协助你们毁灭敌人，今天就让这些害群之马见识见识哈夫克的愤怒和风暴！",
            "我们有责任向阿萨拉人证明，刀锋的空气中充斥着GTI精心编织的谎言。\n\n  我们要用子弹和炮火去问问GTI，是谁导演了凉城假旗行动，致使天网坠毁，大坝决堤？\n\n  谁才是真正将阿萨拉拖入灾难深渊的罪魁祸首？\n\n  谁又会在战火熄灭后留下来与阿萨拉人一起修复这片土地的创伤？\n\n——今天务必让万恶的GTI做出回答。\n",
            "不用理会G.T.I罗织的罪名，新世界的地图如何绘制我们说了算！\n\n  他们已变成了一条疯狗，破坏我们的天网，摧毁我们的运输线路，偷窃哈夫克为打造新世界研制的新型能源……\n\n  这一切只是因为我们动到了他们的蛋糕！\n\n  然而世界需要洗牌，我们需要建立全新的秩序。\n\n  战斗吧！让敌人恐惧，让敌人战栗，让敌人的哀嚎引领我们迈向美好的未来！"
        };
        int idx = (timer_get(TC_TIME2_CH0) % 8); // 用系统tick做伪随机，避免每次都一样
        uart_write_string(UART_INDEX, Haavk_Strings[idx]);
        uart_write_string(UART_INDEX, "\r\n");
    }
    else
    {
        uart_write_string(UART_INDEX, "\033[1;31mUnknown command:\033[0m ");
        uart_write_string(UART_INDEX, cmd);
        uart_write_string(UART_INDEX, "\r\n\033[1;33mType 'help' to see available commands\033[0m\r\n");
    }
}
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       显示所有键值对
// 参数说明       void
// 返回参数       void
//-------------------------------------------------------------------------------------------------------------------
void show_all_kv_pairs(void)
{
    uart_write_string(UART_INDEX, "\r\n\033[1;36m========== All Key-Value Pairs ==========\033[0m\r\n");

    int count = 0;
    for(int i = 0; i < MAX_KV_PAIRS; i++)
    {
        if(KV_List[i].key[0] != '\0')  // Non-empty key
        {
            count++;
            uart_write_string(UART_INDEX, "\033[1;33m");
            char index_str[8];
            sprintf(index_str, "%2d. ", count);
            uart_write_string(UART_INDEX, index_str);
            uart_write_string(UART_INDEX, "\033[1;32m");
            uart_write_string(UART_INDEX, KV_List[i].key);
            uart_write_string(UART_INDEX, "\033[0m = ");

            if(KV_List[i].is_float)
            {
                char float_str[32];
                sprintf(float_str, "\033[1;36m%.6f\033[0m \033[2m(float)\033[0m", KV_List[i].float_value);
                uart_write_string(UART_INDEX, float_str);
            }
            else
            {
                char int_str[32];
                sprintf(int_str, "\033[1;35m%d\033[0m \033[2m(int)\033[0m", KV_List[i].int_value);
                uart_write_string(UART_INDEX, int_str);
            }
            uart_write_string(UART_INDEX, "\r\n");
        }
    }

    if(count == 0)
    {
        uart_write_string(UART_INDEX, "\033[1;31mNo key-value pairs found\033[0m\r\n");
        uart_write_string(UART_INDEX, "\033[1;33mTip:\033[0m Use 'set <key> <value>' to add data\r\n");
    }
    else
    {
        char count_str[64];
        sprintf(count_str, "\r\n\033[1;32mTotal: %d key-value pairs\033[0m\r\n", count);
        uart_write_string(UART_INDEX, count_str);
    }
    uart_write_string(UART_INDEX, "==========================================\r\n");
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       查找指定的键
// 参数说明       key - 要查找的键
// 返回参数       void
//-------------------------------------------------------------------------------------------------------------------
void find_key(char* key)
{
    while(*key == ' ') key++;

    if(strlen(key) == 0)
    {
        uart_write_string(UART_INDEX, "\033[1;31mError:\033[0m Please specify a key to find\r\n");
        uart_write_string(UART_INDEX, "\033[1;33mUsage:\033[0m find <key>\r\n");
        return;
    }

    for(int i = 0; i < MAX_KV_PAIRS; i++)
    {
        if(KV_List[i].key[0] != '\0' && strcmp(KV_List[i].key, key) == 0)
        {
            uart_write_string(UART_INDEX, "\r\n\033[1;32m✓ Key-value pair found:\033[0m\r\n");
            uart_write_string(UART_INDEX, "  \033[1;33mKey:\033[0m \033[1;32m");
            uart_write_string(UART_INDEX, KV_List[i].key);
            uart_write_string(UART_INDEX, "\033[0m\r\n  \033[1;33mValue:\033[0m ");

            if(KV_List[i].is_float)
            {
                char float_str[32];
                sprintf(float_str, "\033[1;36m%.6f\033[0m \033[2m(float)\033[0m", KV_List[i].float_value);
                uart_write_string(UART_INDEX, float_str);
            }
            else
            {
                char int_str[32];
                sprintf(int_str, "\033[1;35m%d\033[0m \033[2m(int)\033[0m", KV_List[i].int_value);
                uart_write_string(UART_INDEX, int_str);
            }
            uart_write_string(UART_INDEX, "\r\n");
            return;
        }
    }

    uart_write_string(UART_INDEX, "\033[1;31m✗ Key not found:\033[0m ");
    uart_write_string(UART_INDEX, key);
    uart_write_string(UART_INDEX, "\r\n\033[1;33mTip:\033[0m Use 'list' to see all available keys\r\n");
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       删除指定的键值对
// 参数说明       key - 要删除的键
// 返回参数       void
//-------------------------------------------------------------------------------------------------------------------
void delete_key(char* key)
{
    while(*key == ' ') key++;

    if(strlen(key) == 0)
    {
        uart_write_string(UART_INDEX, "\033[1;31mError:\033[0m Please specify a key to delete\r\n");
        uart_write_string(UART_INDEX, "\033[1;33mUsage:\033[0m delete <key>\r\n");
        return;
    }

    if(kv_storage_remove(key))
    {
        uart_write_string(UART_INDEX, "\033[1;32m✓ Successfully deleted key-value pair:\033[0m ");
        uart_write_string(UART_INDEX, key);
        uart_write_string(UART_INDEX, "\r\n");
    }
    else
    {
        uart_write_string(UART_INDEX, "\033[1;31m✗ Failed to delete, key not found:\033[0m ");
        uart_write_string(UART_INDEX, key);
        uart_write_string(UART_INDEX, "\r\n\033[1;33mTip:\033[0m Use 'find <key>' to check if key exists\r\n");
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       格式化Flash存储
// 参数说明       void
// 返回参数       void
//-------------------------------------------------------------------------------------------------------------------
void format_flash(void)
{
    uart_write_string(UART_INDEX, "\r\n\033[1;31m⚠️  WARNING: This operation will delete ALL data!\033[0m\r\n");
    uart_write_string(UART_INDEX, "\033[1;33mFormatting Flash storage...\033[0m\r\n");

    if(kv_storage_format())
    {
        uart_write_string(UART_INDEX, "\033[1;32m✓ Flash storage formatted successfully\033[0m\r\n");
        uart_write_string(UART_INDEX, "\033[1;33mTip:\033[0m Storage is now empty. Use 'set <key> <value>' to add data.\r\n");
    }
    else
    {
        uart_write_string(UART_INDEX, "\033[1;31m✗ Failed to format Flash storage\033[0m\r\n");
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       解析设置命令
// 参数说明       cmd - 命令参数字符串 (格式: key value)
// 返回参数       void
//-------------------------------------------------------------------------------------------------------------------
void parse_set_command(char* cmd)
{
    // 去除前导空格
    while(*cmd == ' ') cmd++;
    
    if(strlen(cmd) == 0)
    {
        uart_write_string(UART_INDEX, "\033[1;31mError:\033[0m Missing parameters\r\n");
        uart_write_string(UART_INDEX, "\033[1;33mUsage:\033[0m set <key> <value>\r\n");
        uart_write_string(UART_INDEX, "\033[1;33mExample:\033[0m set temperature 23.5\r\n");
        return;
    }
    
    // 查找第一个空格，分离键和值
    char* space_pos = strchr(cmd, ' ');
    if(space_pos == NULL)
    {
        uart_write_string(UART_INDEX, "\033[1;31mError:\033[0m Missing value parameter\r\n");
        uart_write_string(UART_INDEX, "\033[1;33mUsage:\033[0m set <key> <value>\r\n");
        return;
    }
    
    // 分离键和值
    *space_pos = '\0';  // 将空格替换为字符串结束符
    char* key = cmd;
    char* value_str = space_pos + 1;
    
    // 去除值字符串的前导空格
    while(*value_str == ' ') value_str++;
    
    if(strlen(value_str) == 0)
    {
        uart_write_string(UART_INDEX, "\033[1;31mError:\033[0m Empty value\r\n");
        uart_write_string(UART_INDEX, "\033[1;33mUsage:\033[0m set <key> <value>\r\n");
        return;
    }
    
    // 判断值的类型（整数或浮点数）
    bool is_float = false;
    if(strchr(value_str, '.') != NULL)
    {
        is_float = true;
    }
    
    bool success = false;
    if(is_float)
    {
        float value = atof(value_str);
        success = kv_storage_set_float(key, value);
        if(success)
        {
            uart_write_string(UART_INDEX, "\033[1;32m✓ Successfully set float value:\033[0m ");
            uart_write_string(UART_INDEX, "\033[1;33m");
            uart_write_string(UART_INDEX, key);
            uart_write_string(UART_INDEX, "\033[0m = \033[1;36m");
            char float_str[32];
            sprintf(float_str, "%.6f", value);
            uart_write_string(UART_INDEX, float_str);
            uart_write_string(UART_INDEX, "\033[0m\r\n");
        }
    }
    else
    {
        int value = atoi(value_str);
        success = kv_storage_set_int(key, value);
        if(success)
        {
            uart_write_string(UART_INDEX, "\033[1;32m✓ Successfully set integer value:\033[0m ");
            uart_write_string(UART_INDEX, "\033[1;33m");
            uart_write_string(UART_INDEX, key);
            uart_write_string(UART_INDEX, "\033[0m = \033[1;35m");
            char int_str[32];
            sprintf(int_str, "%d", value);
            uart_write_string(UART_INDEX, int_str);
            uart_write_string(UART_INDEX, "\033[0m\r\n");
        }
    }
    
    if(!success)
    {
        uart_write_string(UART_INDEX, "\033[1;31m✗ Failed to set value\033[0m\r\n");
        uart_write_string(UART_INDEX, "\033[1;33mPossible causes:\033[0m Storage full, invalid key, or system error\r\n");
        uart_write_string(UART_INDEX, "\033[1;33mTip:\033[0m Use 'status' to check storage usage\r\n");
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       显示存储状态信息
// 参数说明       void
// 返回参数       void
//-------------------------------------------------------------------------------------------------------------------
void show_status(void)
{
    uart_write_string(UART_INDEX, "\r\n\033[1;36m========== Storage Status ==========\033[0m\r\n");
    
    // 统计键值对数量
    int used_slots = 0;
    int int_count = 0;
    int float_count = 0;
    
    for(int i = 0; i < MAX_KV_PAIRS; i++)
    {
        if(KV_List[i].key[0] != '\0')
        {
            used_slots++;
            if(KV_List[i].is_float)
                float_count++;
            else
                int_count++;
        }
    }
    
    uart_write_string(UART_INDEX, "\033[1;32mStorage Usage:\033[0m\r\n");
    char stat_str[64];
    sprintf(stat_str, "  Used slots: %d / %d (%.1f%%)\r\n", 
            used_slots, MAX_KV_PAIRS, (float)used_slots * 100 / MAX_KV_PAIRS);
    uart_write_string(UART_INDEX, stat_str);
    
    sprintf(stat_str, "  Integer values: %d\r\n", int_count);
    uart_write_string(UART_INDEX, stat_str);
    
    sprintf(stat_str, "  Float values: %d\r\n", float_count);
    uart_write_string(UART_INDEX, stat_str);
    
    uart_write_string(UART_INDEX, "\r\n\033[1;32mMemory Info:\033[0m\r\n");
    sprintf(stat_str, "  Max pairs: %d\r\n", MAX_KV_PAIRS);
    uart_write_string(UART_INDEX, stat_str);
    
    sprintf(stat_str, "  Memory usage: %d bytes\r\n", (int)sizeof(KV_List));
    uart_write_string(UART_INDEX, stat_str);
    
    if(used_slots == 0)
    {
        uart_write_string(UART_INDEX, "\r\n\033[1;33mTip:\033[0m Storage is empty. Use 'set <key> <value>' to add data.\r\n");
    }
    else if(used_slots >= MAX_KV_PAIRS * 0.8)
    {
        uart_write_string(UART_INDEX, "\r\n\033[1;31mWarning:\033[0m Storage is nearly full!\r\n");
    }
    
    uart_write_string(UART_INDEX, "=====================================\r\n");
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       显示版本信息
// 参数说明       void
// 返回参数       void
//-------------------------------------------------------------------------------------------------------------------
void show_version(void)
{
    uart_write_string(UART_INDEX, "\r\n\033[1;36m========== Version Information ==========\033[0m\r\n");
    uart_write_string(UART_INDEX, "\033[1;32mFlash KV Storage CLI\033[0m\r\n");
    uart_write_string(UART_INDEX, "Version: \033[1;33m2.0 Enhanced\033[0m\r\n");
    uart_write_string(UART_INDEX, "Build Date: \033[1;33m" __DATE__ " " __TIME__ "\033[0m\r\n");
    uart_write_string(UART_INDEX, "Platform: \033[1;33mSeekfree HAL Library\033[0m\r\n");
    uart_write_string(UART_INDEX, "Target: \033[1;33mCYT2BL3 MCU\033[0m\r\n");
    uart_write_string(UART_INDEX, "\r\n\033[1;32mFeatures:\033[0m\r\n");
    uart_write_string(UART_INDEX, "  • Key-Value storage in Flash memory\r\n");
    uart_write_string(UART_INDEX, "  • Auto-type detection (int/float)\r\n");
    uart_write_string(UART_INDEX, "  • Command aliases and tab completion\r\n");
    uart_write_string(UART_INDEX, "  • Colored terminal output\r\n");
    uart_write_string(UART_INDEX, "  • Storage statistics and monitoring\r\n");
    uart_write_string(UART_INDEX, "\r\n\033[1;35mRepository:\033[0m https://github.com/Eclipse-01/Smartcar-Menu-and-Flash-Storage\r\n");
    uart_write_string(UART_INDEX, "==========================================\r\n");
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       清屏
// 参数说明       void
// 返回参数       void
//-------------------------------------------------------------------------------------------------------------------
void clear_screen(void)
{
    // ANSI escape sequence to clear screen and move cursor to top-left
    uart_write_string(UART_INDEX, "\033[2J\033[H");
    uart_write_string(UART_INDEX, "\r\n\033[1;36m===========================================\033[0m\r\n");
    uart_write_string(UART_INDEX, "\033[1;36m    Flash KV Storage Manage CLI v2.0\033[0m\r\n");
    uart_write_string(UART_INDEX, "\033[1;36m===========================================\033[0m\r\n");
    uart_write_string(UART_INDEX, "Type '\033[1;33mhelp\033[0m' to see available commands\r\n");
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       自动完成命令 (基础实现)
// 参数说明       partial - 部分命令字符串
// 返回参数       完整命令字符串或NULL
//-------------------------------------------------------------------------------------------------------------------
char* complete_command(char* partial)
{
    static char* commands[] = {
        "list", "ls", "find", "delete", "rm", "set", "format", 
        "save", "load", "help", "h", "status", "version", "clear", "exit", "quit", NULL
    };
    
    int len = strlen(partial);
    if(len == 0) return NULL;
    
    char* match = NULL;
    int match_count = 0;
    
    for(int i = 0; commands[i] != NULL; i++)
    {
        if(strncmp(commands[i], partial, len) == 0)
        {
            match = commands[i];
            match_count++;
            if(match_count > 1) break; // 多个匹配，不自动完成
        }
    }
    
    return (match_count == 1) ? match : NULL;
}



