#include "Menu_Handler.h"

void EnterMenu(Menu *menu)
{
    // 进入菜单时的初始化操作
    ips200_set_color(TextColor, BackgroundColor);
    DrawMenu(*menu);
    hid_init(); // 初始化HID设备
    while(1) {
        system_delay_ms(100); // 延时，避免过快刷新
        // 处理菜单项的选择和操作
        if (Read_Buttons() == 0) { // 无按键操作
            continue;
        } else if (Read_Buttons() == 1) { // 上键
            if (menu->CurrentSelection > 0) {
                menu->CurrentSelection--;
                DrawMenu(*menu); // 重新绘制菜单
            }
        } else if (Read_Buttons() == 2) { // 下键
            if (menu->CurrentSelection < menu->ItemCount - 1) {
                menu->CurrentSelection++;
                DrawMenu(*menu); // 重新绘制菜单
            }
        } else if (Read_Buttons() == 3) { // 确认键
            // 执行当前选中项的函数
            if (menu->Items[menu->CurrentSelection].ItemFunction != NULL) {
                menu->Items[menu->CurrentSelection].ItemFunction();
                DrawMenu(*menu); // 重新绘制菜单
            }
        } else if (Read_Buttons() == 4) { // 返回键
            // 退出菜单
            ips200_clear(); // 清屏
        }
        printf("Current Selection: %d\n", menu->CurrentSelection); // 调试输出当前选中项
        printf("Button State: %d\n", Read_Buttons()); // 调试输出按键状态  
    }
}
