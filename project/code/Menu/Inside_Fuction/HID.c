#include "HID.h"

#define ENCODER1                     	(TC_CH09_ENCODER)                      // 编码器接口       
#define ENCODER1_DIR_PULSE1            	(TC_CH09_ENCODER_CH1_P05_0)            // A引脚                      
#define ENCODER1_DIR_DIR1             	(TC_CH09_ENCODER_CH2_P05_1)            // B引脚                        
                                                                                
#define ENCODER2                     	(TC_CH07_ENCODER)                      // 编码器接口   
#define ENCODER2_DIR_PULSE2            	(TC_CH07_ENCODER_CH1_P02_0)            // A引脚                  
#define ENCODER2_DIR_DIR2              	(TC_CH07_ENCODER_CH2_P02_1)            // B引脚         

#define KEY1                    (P11_0)
#define KEY2                    (P22_0)
#define KEY3                    (P23_3)
#define KEY4                    (P23_4)

int16 encoder_data_dir[2] = {0};


void hid_init(void)
{
    gpio_init(KEY1, GPI, GPIO_HIGH, GPI_PULL_UP);           // 初始化 KEY1 输入 默认高电平 上拉输入
    gpio_init(KEY2, GPI, GPIO_HIGH, GPI_PULL_UP);           // 初始化 KEY2 输入 默认高电平 上拉输入
    gpio_init(KEY3, GPI, GPIO_HIGH, GPI_PULL_UP);           // 初始化 KEY3 输入 默认高电平 上拉输入
    gpio_init(KEY4, GPI, GPIO_HIGH, GPI_PULL_UP);           // 初始化 KEY4 输入 默认高电平 上拉输入

    encoder_dir_init(ENCODER1, ENCODER1_DIR_PULSE1, ENCODER1_DIR_DIR1);       // 初始化编码器模块与引脚 带方向增量编码器模式
    encoder_dir_init(ENCODER2, ENCODER2_DIR_PULSE2, ENCODER2_DIR_DIR2);       // 初始化编码器模块与引脚 带方向增量编码器模式
                                                  



}

int Get_Encoder_Value(void)
{
    // 获取编码器值
    encoder_data_dir[0] = encoder_get_count(ENCODER1);
    encoder_clear_count(ENCODER1);
    encoder_data_dir[1] = encoder_get_count(ENCODER2);
    encoder_clear_count(ENCODER2);
    return encoder_data_dir[0] / 3;
}

int Key_State = 0;

int Read_Buttons(void)
{
    Key_State = 0; // 重置按键状态
    if(!gpio_get_level(KEY1))
    {
        Key_State = 1;
    }
    else if(!gpio_get_level(KEY2))
    {
        Key_State = 2;
    }
    else if(!gpio_get_level(KEY3))
    {
        Key_State = 3;
    }
    else if(!gpio_get_level(KEY4))
    {
        Key_State = 4;
    }
    return Key_State; // 返回按键状态
}
