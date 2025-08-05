#include "BasicGeos.h"

void draw_rectangle(int x, int y, int width, int height, uint32 color) {
    // 绘制矩形边框
    ips200_draw_line(x, y, x + width, y, color); // 上边
    ips200_draw_line(x + width, y, x + width, y + height, color); // 右边
    ips200_draw_line(x + width, y + height, x, y + height, color); // 下边
    ips200_draw_line(x, y + height, x, y, color); // 左边
}

void draw_rectangle_filled(int x, int y, int width, int height, uint32 color) {
    for (int i = 0; i < height; i++) {
        ips200_draw_line(x, y + i, x + width, y + i, color);
    }
}

