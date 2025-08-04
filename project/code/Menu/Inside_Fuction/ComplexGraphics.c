#include "ComplexGraphics.h"
#include "Items.h"
#include "BasicGeos.h"
#include "Theme.h"
#include "zf_common_headfile.h"

void DrawMenu(Menu menu)
{
    // 绘制菜单标题，居中
    int title_length = strlen(menu.MenuTitle) * 8;
    ips200_full(RGB565_BLACK); // 清屏
    ips200_set_color(TextColor, BackgroundColor);
    ips200_show_string((240 - title_length) / 2, 25, menu.MenuTitle);
    ips200_draw_line(0, 45, 239, 45, TitleSeperatorColor); // 绘制标题下划线

    // 绘制菜单项
    for (int i = 0; i < menu.ItemCount || i < MAX_ITEMS_PER_PAGE; i++) {
        printf("i = %d, ItemCount = %d\n", i, menu.ItemCount);
        int item_y = 60 + i * 24; // 计算每个菜单项的 y 坐标
        int item_x = 210 - strlen(menu.Items[i].ItemValue) * 8;
        if (i == menu.CurrentSelection) {
            ips200_set_color(Text_Selected_Color, Background_Selected_Color); // 选中项颜色
            draw_rectangle_filled(0, item_y , 239, 18, Background_Selected_Color); // 绘制选中项背景
            ips200_show_char(15, item_y, '>'); // 显示选中标记
            ips200_show_string(30, item_y, menu.Items[i].ItemName); // 显示菜单项名称
            ips200_show_string(item_x, item_y, menu.Items[i].ItemValue); // 显示菜单项值
        }
        else {
        ips200_set_color(TextColor, BackgroundColor); // 非选中项颜色
        ips200_show_string(30, item_y, menu.Items[i].ItemName); // 显示菜单项名称
        ips200_show_string(item_x, item_y, menu.Items[i].ItemValue); // 显示菜单项值
        }
    }
}

void SetValueComponent_Int(Menu menu, int item_index){
    int item_y = 60 + item_index * 24; // 计算每个菜单项的 y 坐标
    int item_x = 210 - strlen(menu.Items[item_index].ItemValue) * 8;
    ips200_set_color(ChangeValueTextColor, Background_Selected_Color);
    ips200_show_string(item_x, item_y, menu.Items[item_index].ItemValue); // 显示菜单项值
    int Value = atoi(menu.Items[item_index].ItemValue); // 获取当前值
    int bit = 0;
    int Lenth = 0;
    char data_buffer[10];
    char current_char = '0'; // 当前字符

    // 接下来处理值的修改
    while(1){
        // 查看当前数的长度
        sprintf(data_buffer, "%d", Value);
        Lenth = strlen(data_buffer);

        // 特殊显示当前的步长Bit
        current_char = data_buffer[Lenth - 1 - bit];
        ips200_set_color(ChangeValueText_Selected_BitColor, ChangeValueText_Selected_Background_Color);
        ips200_show_char(item_x + bit * 8, item_y, current_char);

        if (Get_Encoder_Value() != 0) {
            // 将Value的值的位幂次加上编码器的值
            Value += Get_Encoder_Value() * pow(10, bit);
        }

        // 处理按键输入
        if (Read_Buttons() == 1) {
            // 将Bit移到高位
            bit++;
            if (bit >= Lenth) {
                bit = Lenth - 1; // 限制Bit不超过当前值的长度
            }
        }
        if (Read_Buttons() == 2) {
            // 将Bit移到低位
            bit--;
            if (bit < 0) {
                bit = 0;
            }
        }
        if (Read_Buttons() == 3) {
            strcpy(menu.Items[item_index].ItemValue, data_buffer); // 更新菜单项的值
            break;
        }
        if (Read_Buttons() == 4) {
            // 取消修改，退出编辑
            return;
        }
    }
}

void SetValueComponent_Float(Menu menu, int item_index){
    int item_y = 60 + item_index * 24; // 计算每个菜单项的 y 坐标
    int item_x = 210 - strlen(menu.Items[item_index].ItemValue) * 8;
    ips200_set_color(ChangeValueTextColor, Background_Selected_Color);
    ips200_show_string(item_x, item_y, menu.Items[item_index].ItemValue); // 显示菜单项值
    double Value = atof(menu.Items[item_index].ItemValue);
    int bit = 0;
    int Length = 0;
    char data_buffer[20];
    char current_char = '0'; // 当前字符

    // 接下来处理值的修改
    while(1){
        // 查看当前数的长度
        sprintf(data_buffer, "%.2f", Value);
        Length = strlen(data_buffer);

        // 特殊显示当前的步长Bit
        current_char = data_buffer[Length - 1 - bit];
        ips200_set_color(ChangeValueText_Selected_BitColor, ChangeValueText_Selected_Background_Color);
        ips200_show_char(item_x + bit * 8, item_y, current_char);

        if (Get_Encoder_Value() != 0) {
            // 将Value的值的位幂次加上编码器的值
            Value += Get_Encoder_Value() * pow(10, bit);
        }

        // 处理按键输入
        if (Read_Buttons() == 1) {
            // 将Bit移到高位
            bit++;
            if (bit >= Length) {
                bit = Length - 1; // 限制Bit不超过当前值的长度
            }
        }
        if (Read_Buttons() == 2) {
            // 将Bit移到低位
            bit--;
            if (bit < 0) {
                bit = 0;
            }
        }
        if (Read_Buttons() == 3) {
            strcpy(menu.Items[item_index].ItemValue, data_buffer); // 更新菜单项的值
            break;
        }
        if (Read_Buttons() == 4) {
            // 取消修改，退出编辑
            return;
        }
    }
}
