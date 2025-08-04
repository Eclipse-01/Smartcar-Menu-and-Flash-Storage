#include "ComplexGraphics.h"
#include "Items.h"
#include "BasicGeos.h"
#include "Theme.h"
#include "HID.h"
#include "zf_common_headfile.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ===================================================================
//                         宏定义与前向声明
// ===================================================================

// 为按键定义宏，增强代码可读性
#define KEY_RIGHT   1 // 在原代码中，此为移到高位
#define KEY_LEFT    2 // 在原代码中，此为移到低位
#define KEY_CONFIRM 3
#define KEY_CANCEL  4

// 函数前向声明
void DrawMenu(const Menu* menu);
void SetValueComponent_Int(Menu* menu, int item_index);
void SetValueComponent_Float(Menu* menu, int item_index);
void SetValueComponent_Bool(Menu* menu, int item_index);
void EnterSubMenu(Menu* menu, int item_index);
void ExecuteFunction(Menu* menu, int item_index);
static double powerOf10(int exp);


// ===================================================================
//                         核心功能函数
// ===================================================================

/**
 * @brief 绘制完整的菜单界面.
 * @param menu 指向菜单结构体的常量指针.
 */
void DrawMenu(const Menu* menu)
{
    // 1. 绘制标题
    ips200_full(BackgroundColor); // 使用主题背景色清屏
    ips200_set_color(TextColor, BackgroundColor);
    int title_length = strlen(menu->MenuTitle) * 8;
    ips200_show_string((240 - title_length) / 2, 25, menu->MenuTitle);
    ips200_draw_line(0, 45, 239, 45, TitleSeperatorColor);

    // 2. 计算并显示当前页的菜单项
    // 计算当前页的起始项索引
    int start_item = menu->CurrentPage * MAX_ITEMS_PER_PAGE;
    int end_item = start_item + MAX_ITEMS_PER_PAGE;
    if (end_item > menu->ItemCount) {
        end_item = menu->ItemCount;
    }
    
    int display_index = 0; // 显示位置索引
    for (int i = start_item; i < end_item; i++) {
        int item_y = 60 + display_index * 24;
        const char* item_name = menu->Items[i].ItemName;
        const char* item_value = menu->Items[i].ItemValue;
        int value_x = 210 - strlen(item_value) * 8;

        if (i == menu->CurrentSelection) {
            // 绘制选中项
            ips200_set_color(Text_Selected_Color, Background_Selected_Color);
            draw_rectangle_filled(0, item_y - 2, 239, 20, Background_Selected_Color); // 高亮背景条
            ips200_show_char(15, item_y, '>');
            ips200_show_string(30, item_y, item_name);
            ips200_show_string(value_x, item_y, item_value);
        } else {
            // 绘制非选中项
            ips200_set_color(TextColor, BackgroundColor);
            ips200_show_string(30, item_y, item_name);
            ips200_show_string(value_x, item_y, item_value);
        }
        display_index++;
    }
    // 3. 绘制页码信息
    ips200_set_color(TextColor, BackgroundColor);
    char page_info[20];
    sprintf(page_info, "%d of %d", menu->CurrentPage + 1, (menu->ItemCount + MAX_ITEMS_PER_PAGE - 1) / MAX_ITEMS_PER_PAGE);
    ips200_show_string(98, 280, page_info);
}


/**
 * @brief 参数编辑组件 - 整型.
 * @param menu 指向菜单结构体的指针，用于修改其内部值.
 * @param item_index 要修改的菜单项索引.
 */
void SetValueComponent_Int(Menu* menu, int item_index)
{
    // 计算当前项在当前页面中的显示位置
    int display_index = item_index - (menu->CurrentPage * MAX_ITEMS_PER_PAGE);
    int item_y = 60 + display_index * 24;
    long value = atol(menu->Items[item_index].ItemValue);
    int bit = 0; // 光标位置，从右到左，0为个位

    char data_buffer[20];
    int length;

    system_delay_ms(250); // 消抖

    while (1) {
        // --- 1. 数据转字符串并计算显示位置 ---
        sprintf(data_buffer, "%ld", value);
        length = strlen(data_buffer);
        int item_x = 210 - length * 8;

        // --- 2. 刷新显示 ---
        // a. 清除旧值区域
        draw_rectangle_filled(item_x - 8, item_y, 239 - (item_x - 8), 18, Background_Selected_Color);
        // b. 显示新值
        ips200_set_color(ChangeValueTextColor, Background_Selected_Color);
        ips200_show_string(item_x, item_y, data_buffer);
        // c. 高亮当前编辑的位
        int cursor_idx = length - 1 - bit;
        int char_x = item_x + cursor_idx * 8;
        char current_char = data_buffer[cursor_idx];
        ips200_set_color(ChangeValueText_Selected_BitColor, ChangeValueText_Selected_Background_Color);
        ips200_show_char(char_x, item_y, current_char);

        // --- 3. 等待输入 ---
        int encoder_delta = 0;
        int button_press = 0;
        while (encoder_delta == 0 && button_press == 0) {
            encoder_delta = Get_Encoder_Value();
            button_press = Read_Buttons();
            system_delay_ms(20); // 降低CPU占用
        }

        // --- 4. 处理输入 ---
        if (encoder_delta != 0) {
            long increment = 1;
            for (int i = 0; i < bit; i++) {
                increment *= 10;
            }
            value += encoder_delta * increment;
        }

        if (button_press == KEY_RIGHT) { // 移至高位 (向左)
            if (bit < length - 1) {
                // 如果下一位是负号，则跳过
                if (data_buffer[length - 2 - bit] == '-') {
                    if (bit < length - 2) bit++;
                }
                bit++;
            }
            system_delay_ms(50);
        } else if (button_press == KEY_LEFT) { // 移至低位 (向右)
            if (bit > 0) {
                bit--;
            }
            system_delay_ms(50);
        } else if (button_press == KEY_CONFIRM) { // 确认
            sprintf(menu->Items[item_index].ItemValue, "%ld", value);
            return;
        } else if (button_press == KEY_CANCEL) { // 取消
            return;
        }
    }
}


/**
 * @brief 参数编辑组件 - 浮点型 (优化版).
 * 支持动态调整编辑精度.
 * @param menu 指向菜单结构体的指针，用于修改其内部值.
 * @param item_index 要修改的菜单项索引.
 */
void SetValueComponent_Float(Menu* menu, int item_index)
{
    // --- 1. 初始化 ---
    // 计算当前项在当前页面中的显示位置
    int display_index = item_index - (menu->CurrentPage * MAX_ITEMS_PER_PAGE);
    int item_y = 60 + display_index * 24;
    double value = atof(menu->Items[item_index].ItemValue);
    int bit = 0; // 光标位置，从最右侧的0开始

    // 动态确定初始精度
    int precision = 0;
    char* dot_ptr = strchr(menu->Items[item_index].ItemValue, '.');
    if (dot_ptr != NULL) {
        precision = strlen(dot_ptr + 1);
    }
    const int MAX_PRECISION = 7; // 设定最大精度以防溢出 (e.g., xxx.1234567)

    char data_buffer[20];
    char fmt_buffer[8];
    int length;

    system_delay_ms(250); // 消抖

    while (1) {
        // --- 2. 数据转字符串并计算显示位置 ---
        sprintf(fmt_buffer, "%%.%df", precision); // 创建格式化字符串, e.g., "%.3f"
        sprintf(data_buffer, fmt_buffer, value);
        length = strlen(data_buffer);
        int item_x = 210 - length * 8;

        // --- 3. 刷新显示 ---
        draw_rectangle_filled(item_x - 8, item_y, 239 - (item_x - 8), 18, Background_Selected_Color);
        ips200_set_color(ChangeValueTextColor, Background_Selected_Color);
        ips200_show_string(item_x, item_y, data_buffer);

        int cursor_idx = length - 1 - bit;
        int char_x = item_x + cursor_idx * 8;
        char current_char = data_buffer[cursor_idx];
        ips200_set_color(ChangeValueText_Selected_BitColor, ChangeValueText_Selected_Background_Color);
        ips200_show_char(char_x, item_y, current_char);

        // --- 4. 等待输入 ---
        int encoder_delta = 0;
        int button_press = 0;
        while (encoder_delta == 0 && button_press == 0) {
            encoder_delta = Get_Encoder_Value();
            button_press = Read_Buttons();
            system_delay_ms(20);
        }

        // --- 5. 处理输入 ---
        if (encoder_delta != 0) {
            dot_ptr = strchr(data_buffer, '.');
            int dot_offset = 0;
            if (dot_ptr != NULL) {
                dot_offset = (data_buffer + length - 1) - dot_ptr;
            }
            double increment = powerOf10(bit - dot_offset);
            value += encoder_delta * increment;
        }

        if (button_press == KEY_RIGHT) { // 移至高位 (向左)
            if (bit < length - 1) {
                bit++;
                // 如果移动后是小数点或负号，则再移动一次跳过它
                char next_char = data_buffer[length - 1 - bit];
                if (next_char == '.' || next_char == '-') {
                    if (bit < length - 1) {
                        bit++;
                    }
                }
            }
            system_delay_ms(100);
        } else if (button_press == KEY_LEFT) { // 移至低位 (向右)
            // ** 核心功能：当在最右侧时，增加精度 **
            if (bit == 0) {
                if (precision < MAX_PRECISION) {
                    precision++;
                    // value不变，循环会自动补零并重绘
                }
            } else {
                bit--;
                // 如果移动后是小数点，则再移动一次跳过它
                if (data_buffer[length - 1 - bit] == '.') {
                    if (bit > 0) {
                        bit--;
                    }
                }
            }
            system_delay_ms(100);
        } else if (button_press == KEY_CONFIRM) { // 确认
            sprintf(fmt_buffer, "%%.%df", precision);
            sprintf(menu->Items[item_index].ItemValue, fmt_buffer, value);
            return;
        } else if (button_press == KEY_CANCEL) { // 取消
            return;
        }
    }
}


// ===================================================================
//                           辅助工具函数
// ===================================================================

/**
 * @brief 高效计算10的幂，支持正负指数.
 * @param exp 指数.
 * @return 10的exp次幂.
 */
static double powerOf10(int exp)
{
    double result = 1.0;
    if (exp > 0) {
        for (int i = 0; i < exp; i++) {
            result *= 10.0;
        }
    } else if (exp < 0) {
        for (int i = 0; i < -exp; i++) {
            result /= 10.0;
        }
    }
    return result;
}


/**
 * @brief 布尔值编辑组件.
 * @param menu 指向菜单结构体的指针，用于修改其内部值.
 * @param item_index 要修改的菜单项索引.
 */
void SetValueComponent_Bool(Menu* menu, int item_index)
{
    // 计算当前项在当前页面中的显示位置
    int display_index = item_index - (menu->CurrentPage * MAX_ITEMS_PER_PAGE);
    int item_y = 60 + display_index * 24;
    
    // 获取当前值 (假设 "0" 为 false, "1" 为 true)
    int current_value = atoi(menu->Items[item_index].ItemValue);
    
    system_delay_ms(250); // 消抖

    while (1) {
        // --- 1. 准备显示文本 ---
        const char* display_text = current_value ? "TRUE" : "FALSE";
        int text_length = strlen(display_text);
        int item_x = 210 - text_length * 8;

        // --- 2. 刷新显示 ---
        // 清除旧值区域
        draw_rectangle_filled(item_x - 8, item_y, 239 - (item_x - 8), 18, Background_Selected_Color);
        // 显示新值
        ips200_set_color(ChangeValueTextColor, Background_Selected_Color);
        ips200_show_string(item_x, item_y, display_text);

        // --- 3. 等待输入 ---
        int encoder_delta = 0;
        int button_press = 0;
        while (encoder_delta == 0 && button_press == 0) {
            encoder_delta = Get_Encoder_Value();
            button_press = Read_Buttons();
            system_delay_ms(20);
        }

        // --- 4. 处理输入 ---
        if (encoder_delta != 0) {
            // 旋转编码器切换布尔值
            current_value = current_value ? 0 : 1;
        }

        if (button_press == KEY_RIGHT || button_press == KEY_LEFT) {
            // 左右键也可以切换布尔值
            current_value = current_value ? 0 : 1;
            system_delay_ms(100);
        } else if (button_press == KEY_CONFIRM) { // 确认
            sprintf(menu->Items[item_index].ItemValue, "%d", current_value);
            return;
        } else if (button_press == KEY_CANCEL) { // 取消
            return;
        }
    }
}


/**
 * @brief 进入子菜单.
 * @param menu 指向当前菜单结构体的指针.
 * @param item_index 要进入的子菜单项索引.
 */
void EnterSubMenu(Menu* menu, int item_index)
{
    MenuItem* current_item = &menu->Items[item_index];
    
    // 检查是否有有效的子菜单
    if (current_item->SubMenu == NULL) {
        // 如果没有子菜单，显示错误信息
        ips200_full(BackgroundColor);
        ips200_set_color(TextColor, BackgroundColor);
        ips200_show_string(60, 120, "No SubMenu Available");
        system_delay_ms(1000);
        return;
    }
    
    // 保存当前菜单状态 (如果需要的话)
    // 这里可以添加一个菜单栈来支持多级返回
    
    // 递归调用 EnterMenu 进入子菜单
    // 注意：这需要在 Menu_Handler.c 中声明 EnterMenu 函数
    extern void EnterMenu(Menu *menu);
    EnterMenu(current_item->SubMenu);
    
    // 从子菜单返回后，重新绘制当前菜单
    DrawMenu(menu);
}


/**
 * @brief 执行菜单项关联的函数.
 * @param menu 指向菜单结构体的指针.
 * @param item_index 要执行函数的菜单项索引.
 */
void ExecuteFunction(Menu* menu, int item_index)
{
    MenuItem* current_item = &menu->Items[item_index];
    
    // 检查是否有有效的函数指针
    if (current_item->ItemFunction == NULL) {
        // 如果没有函数，显示错误信息
        ips200_full(BackgroundColor);
        ips200_set_color(TextColor, BackgroundColor);
        ips200_show_string(60, 120, "No Function Available");
        system_delay_ms(1000);
        DrawMenu(menu); // 重新绘制菜单
        return;
    }
    
    // 显示执行提示
    ips200_full(BackgroundColor);
    ips200_set_color(TextColor, BackgroundColor);
    ips200_show_string(80, 100, "Executing...");
    ips200_show_string(60, 120, current_item->ItemName);
    
    // 执行函数
    current_item->ItemFunction();
    
    // 执行完成后显示提示
    ips200_show_string(80, 140, "Completed!");
    system_delay_ms(1000);
    
    // 重新绘制菜单
    DrawMenu(menu);
}
