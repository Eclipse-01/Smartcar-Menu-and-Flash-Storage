#ifndef HID_H
#define HID_H

#include "Configurator_Headfile.h"

extern int16 encoder_data_dir[2]; // 编码器数据数组

// HID 相关函数声明
void hid_init(void);
int Get_Encoder_Value(void);
int Read_Buttons(void);

#endif // HID_H