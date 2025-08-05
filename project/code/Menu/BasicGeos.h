// 这个文件绘制一些基本的集合几何图形
#ifndef BASICGEOS_H
#define BASICGEOS_H

#include "zf_common_headfile.h"
#include "zf_device_ips200.h"
#include "math.h"

void draw_rectangle(int x, int y, int width, int height, uint32 color);
void draw_rectangle_filled(int x, int y, int width, int height, uint32 color);


#endif // BASICGEOS_H