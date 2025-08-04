#include "Menu_Handler.h"
#include "ComplexGraphics.h"
#include "HID.h"
#include "KV_Storage.h"
#include <stdio.h>
#include <stdlib.h>

// 全局变量用于跟踪菜单层级
static int menu_depth = 0;

// 前向声明
static bool show_save_confirm_dialog(void);
static void sync_menu_to_kv_storage(const Menu* menu);
static void sync_kv_storage_to_menu(Menu* menu);
static void sync_kv_storage_to_menu_recursive(Menu* menu);

void EnterMenu(Menu *menu)
{
    // 增加菜单深度
    menu_depth++;
    
    // 如果是最上级菜单，从KV存储同步数据到菜单
    if (menu_depth == 1) {
        printf("正在从Flash加载设置数据...\n");
        kv_load_data(); // 从Flash加载数据
        printf("Flash数据加载完成，开始同步到菜单...\n");
        sync_kv_storage_to_menu_recursive(menu); // 递归同步所有子菜单
        printf("Flash数据加载完成\n");
    }
    
    // 进入菜单时的初始化操作
    ips200_set_color(TextColor, BackgroundColor);
    DrawMenu(menu);
    hid_init(); // 初始化HID设备
    while(1) {
        // 处理菜单项的选择和操作
        if (Read_Buttons() == 0) { // 无按键操作
            int EncoderValue = Get_Encoder_Value() / 30;
        if (EncoderValue != 0) {
            // 处理编码器旋转
            printf("Encoder Value: %d\n", EncoderValue); // 调试输出编码器值
            menu->CurrentSelection += EncoderValue;
            if (menu->CurrentSelection < 0) {
                menu->CurrentSelection = 0; // 限制最小值
            }
            if (menu->CurrentSelection >= menu->ItemCount) {
                menu->CurrentSelection = menu->ItemCount - 1; // 限制最大值
            }
            
            // 自动翻页逻辑
            int total_pages = (menu->ItemCount + MAX_ITEMS_PER_PAGE - 1) / MAX_ITEMS_PER_PAGE;
            int target_page = menu->CurrentSelection / MAX_ITEMS_PER_PAGE;
            if (target_page >= total_pages) {
                target_page = total_pages - 1;
            }
            menu->CurrentPage = target_page;
            
            DrawMenu(menu); // 重新绘制菜单
        }
        } else if (Read_Buttons() == 1) { // 上键
            if (menu->CurrentSelection > 0) {
                menu->CurrentSelection--;
                
                // 自动翻页逻辑
                int target_page = menu->CurrentSelection / MAX_ITEMS_PER_PAGE;
                menu->CurrentPage = target_page;
                
                DrawMenu(menu); // 重新绘制菜单
            }
        } else if (Read_Buttons() == 2) { // 下键
            if (menu->CurrentSelection < menu->ItemCount - 1) {
                menu->CurrentSelection++;
                
                // 自动翻页逻辑
                int target_page = menu->CurrentSelection / MAX_ITEMS_PER_PAGE;
                menu->CurrentPage = target_page;
                
                DrawMenu(menu); // 重新绘制菜单
            }
        } else if (Read_Buttons() == 3) { // 确认键
            // 执行当前选中项的函数
            if (menu->Items[menu->CurrentSelection].Type == ITEM_TYPE_INT) {
                SetValueComponent_Int(menu, menu->CurrentSelection);
            }
            else if (menu->Items[menu->CurrentSelection].Type == ITEM_TYPE_FLOAT) {
                SetValueComponent_Float(menu, menu->CurrentSelection);
            }
            else if (menu->Items[menu->CurrentSelection].Type == ITEM_TYPE_BOOL) {
                SetValueComponent_Bool(menu, menu->CurrentSelection);
            }
            else if (menu->Items[menu->CurrentSelection].Type == ITEM_TYPE_MENU) {
                EnterSubMenu(menu, menu->CurrentSelection);
            }
            else if (menu->Items[menu->CurrentSelection].Type == ITEM_TYPE_FUNCTION) {
                ExecuteFunction(menu, menu->CurrentSelection);
            }
            DrawMenu(menu); // 重新绘制菜单
        }
         else if (Read_Buttons() == 4) { // 返回键
            // 如果是最上级菜单，显示保存确认对话框
            if (menu_depth == 1) {
                if (show_save_confirm_dialog()) {
                    // 用户选择保存
                    printf("正在保存设置到Flash...\n");
                    sync_menu_to_kv_storage(menu);
                    kv_save_data(); // 保存到Flash
                    printf("设置保存完成\n");
                    ips200_full(BackgroundColor);
                    ips200_set_color(TextColor, BackgroundColor);
                    ips200_show_string(80, 120, "Settings Saved!");
                    system_delay_ms(1000);
                } else {
                    printf("用户选择不保存设置\n");
                }
            }
            
            // 减少菜单深度并退出
            menu_depth--;
            ips200_clear(); // 清屏
            return; // 退出函数
        }
        
        printf("Current Selection: %d\n", menu->CurrentSelection); // 调试输出当前选中项
        printf("Button State: %d\n", Read_Buttons()); // 调试输出按键状态  
    }
}


/**
 * @brief 显示保存确认对话框
 * @return true 如果用户选择保存，false 如果用户选择不保存
 */
static bool show_save_confirm_dialog(void)
{
    int selection = 0; // 0 = Yes, 1 = No
    
    system_delay_ms(250); // 消抖
    
    while (1) {
        // 绘制对话框
        ips200_full(BackgroundColor);
        ips200_set_color(TextColor, BackgroundColor);
        
        // 绘制边框
        ips200_draw_line(40, 80, 199, 80, TextColor);
        ips200_draw_line(40, 80, 40, 180, TextColor);
        ips200_draw_line(199, 80, 199, 180, TextColor);
        ips200_draw_line(40, 180, 199, 180, TextColor);
        
        // 显示标题
        ips200_show_string(70, 90, "Save Settings?");
        
        // 显示选项
        if (selection == 0) {
            ips200_set_color(Text_Selected_Color, Background_Selected_Color);
            draw_rectangle_filled(50, 120, 50, 18, Background_Selected_Color);
            ips200_show_string(60, 122, "> Yes");
            ips200_set_color(TextColor, BackgroundColor);
            ips200_show_string(60, 142, "  No");
        } else {
            ips200_set_color(TextColor, BackgroundColor);
            ips200_show_string(60, 122, "  Yes");
            ips200_set_color(Text_Selected_Color, Background_Selected_Color);
            draw_rectangle_filled(50, 140, 50, 18, Background_Selected_Color);
            ips200_show_string(60, 142, "> No");
        }
        
        // 等待用户输入
        int encoder_delta = 0;
        int button_press = 0;
        while (encoder_delta == 0 && button_press == 0) {
            encoder_delta = Get_Encoder_Value();
            button_press = Read_Buttons();
            system_delay_ms(20);
        }
        
        // 处理输入
        if (encoder_delta != 0 || button_press == 1 || button_press == 2) {
            // 切换选择
            selection = (selection == 0) ? 1 : 0;
            system_delay_ms(100);
        } else if (button_press == 3) { // 确认
            return (selection == 0); // Yes = true, No = false
        } else if (button_press == 4) { // 取消 (默认不保存)
            return false;
        }
    }
}


/**
 * @brief 将菜单数据同步到KV存储（递归处理所有子菜单）
 * @param menu 要同步的菜单
 */
static void sync_menu_to_kv_storage(const Menu* menu)
{
    // 处理当前菜单的所有项
    for (int i = 0; i < menu->ItemCount; i++) {
        const MenuItem* item = &menu->Items[i];
        
        // 只同步数值类型的菜单项，跳过子菜单和函数类型
        if (item->Type == ITEM_TYPE_INT) {
            int value = atoi(item->ItemValue);
            kv_storage_set_int(item->ItemName, value);
            printf("保存INT: %s = %d\n", item->ItemName, value);
        } else if (item->Type == ITEM_TYPE_FLOAT) {
            float value = atof(item->ItemValue);
            kv_storage_set_float(item->ItemName, value);
            printf("保存FLOAT: %s = %.3f\n", item->ItemName, value);
        } else if (item->Type == ITEM_TYPE_BOOL) {
            int value = atoi(item->ItemValue);
            kv_storage_set_int(item->ItemName, value);
            printf("保存BOOL: %s = %d\n", item->ItemName, value);
        } else if (item->Type == ITEM_TYPE_MENU && item->SubMenu != NULL) {
            // 递归处理子菜单，但不保存子菜单本身
            sync_menu_to_kv_storage(item->SubMenu);
        }
        // ITEM_TYPE_FUNCTION 类型被跳过，不保存到Flash
    }
}


/**
 * @brief 将KV存储数据同步到菜单（单层）
 * @param menu 要同步的菜单
 */
static void sync_kv_storage_to_menu(Menu* menu)
{
    for (int i = 0; i < menu->ItemCount; i++) {
        MenuItem* item = &menu->Items[i];
        
        // 只同步数值类型的菜单项
        if (item->Type == ITEM_TYPE_INT || item->Type == ITEM_TYPE_BOOL) {
            int value;
            if (kv_storage_get_int(item->ItemName, &value)) {
                sprintf(item->ItemValue, "%d", value);
                printf("加载INT/BOOL: %s = %d\n", item->ItemName, value);
            }
        } else if (item->Type == ITEM_TYPE_FLOAT) {
            float value;
            if (kv_storage_get_float(item->ItemName, &value)) {
                sprintf(item->ItemValue, "%.3f", value);
                printf("加载FLOAT: %s = %.3f\n", item->ItemName, value);
            }
        }
    }
}


/**
 * @brief 将KV存储数据同步到菜单（递归处理所有子菜单）
 * @param menu 要同步的菜单
 */
static void sync_kv_storage_to_menu_recursive(Menu* menu)
{
    // 处理当前菜单的所有项
    for (int i = 0; i < menu->ItemCount; i++) {
        MenuItem* item = &menu->Items[i];
        
        // 只同步数值类型的菜单项
        if (item->Type == ITEM_TYPE_INT || item->Type == ITEM_TYPE_BOOL) {
            int value;
            if (kv_storage_get_int(item->ItemName, &value)) {
                sprintf(item->ItemValue, "%d", value);
                printf("加载INT/BOOL: %s = %d\n", item->ItemName, value);
            }
        } else if (item->Type == ITEM_TYPE_FLOAT) {
            float value;
            if (kv_storage_get_float(item->ItemName, &value)) {
                sprintf(item->ItemValue, "%.3f", value);
                printf("加载FLOAT: %s = %.3f\n", item->ItemName, value);
            }
        } else if (item->Type == ITEM_TYPE_MENU && item->SubMenu != NULL) {
            // 递归处理子菜单
            sync_kv_storage_to_menu_recursive(item->SubMenu);
        }
        // ITEM_TYPE_FUNCTION 类型被跳过，不从Flash加载
    }
}
