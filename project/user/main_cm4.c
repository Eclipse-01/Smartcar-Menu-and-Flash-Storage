/*********************************************************************************************************************
* CYT2BL3 Opensourec Library 即（ CYT2BL3 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 本文件是 CYT2BL3 开源库的一部分
*
* CYT2BL3 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 本开源库的发布是希望它能发挥作用，但并未对其作任何的保证
* 甚至没有隐含的适销性或适合特定用途的保证
* 更多细节请参见 GPL
*
* 您应该在收到本开源库的同时收到一份 GPL 的副本
* 如果没有，请参阅<https://www.gnu.org/licenses/>
*
* 额外注明：
* 本开源库使用 GPL3.0 开源许可证协议 以上许可申明为译文版本
* 许可申明英文版在 libraries/doc 文件夹下的 GPL3_permission_statement.txt 文件中
* 许可证副本在 libraries 文件夹下 即该文件夹下的 LICENSE 文件
* 欢迎各位使用并传播本程序 但修改内容时必须保留逐飞科技的版权声明（即本声明）
*
* 文件名称          main_cm4
* 公司名称          成都逐飞科技有限公司
* 版本信息          查看 libraries/doc 文件夹内 version 文件 版本说明
* 开发环境          IAR 9.40.1
* 适用平台          CYT2BL3
* 店铺链接          https://seekfree.taobao.com/
*
* 修改记录
* 日期              作者                备注
* 2024-11-19       pudding            first version
********************************************************************************************************************/

#include "zf_common_headfile.h"
#include "Configurator_Headfile.h"

// 打开新的工程或者工程移动了位置务必执行以下操作
// 第一步 关闭上面所有打开的文件
// 第二步 project->clean  等待下方进度条走完

// 本例程是开源库空工程 可用作移植或者测试各类内外设
// 本例程是开源库空工程 可用作移植或者测试各类内外设
// 本例程是开源库空工程 可用作移植或者测试各类内外设

// **************************** 代码区域 ****************************

// 示例函数
void TestFunction1(void)
{
    printf("Test Function 1 executed!\n");
    // 这里可以添加任何你想要执行的代码
}

void TestFunction2(void)
{
    printf("Test Function 2 executed!\n");
    // 这里可以添加任何你想要执行的代码
}

void SystemReset(void)
{
    printf("System Reset Function called!\n");
    // 这里可以添加系统重置代码
}

int main(void)
{
    clock_init(SYSTEM_CLOCK_160M);      // 时钟配置及系统初始化<务必保留>
    
    debug_init();                       // 调试串口初始化
    
    // 初始化KV存储系统
    kv_storage_init();
    
    // 此处编写用户代码 例如外设初始化代码等
    ips200_init(IPS200_TYPE_SPI);                   // IPS200 初始化
    ips200_clear();                    // 清屏
    
    // 创建子菜单示例
    static Menu SettingsMenu = {
        .MenuTitle = "Settings",
        .Items = {
            {"Brightness", "50", ITEM_TYPE_INT, 0, 100, NULL, NULL},
            {"Volume", "75", ITEM_TYPE_INT, 0, 100, NULL, NULL},
            {"Auto Save", "1", ITEM_TYPE_BOOL, 0, 1, NULL, NULL},
            {"Reset Config", "", ITEM_TYPE_FUNCTION, 0, 0, NULL, SystemReset}
        },
        .ItemCount = 4,
        .CurrentPage = 0,
        .CurrentSelection = 0
    };
    
    Menu MainMenu = {
        .MenuTitle = "Main Menu",
        .Items = {
            {"Speed", "1", ITEM_TYPE_INT, -100, 100, NULL, NULL},
            {"Xiaomi", "2.33", ITEM_TYPE_FLOAT, 0, 100, NULL, NULL},
            {"Power On", "1", ITEM_TYPE_BOOL, 0, 1, NULL, NULL},
            {"Settings", "", ITEM_TYPE_MENU, 0, 0, &SettingsMenu, NULL},
            {"Test Func1", "", ITEM_TYPE_FUNCTION, 0, 0, NULL, TestFunction1},
            {"Test Func2", "", ITEM_TYPE_FUNCTION, 0, 0, NULL, TestFunction2},
            {"Temperature", "25.5", ITEM_TYPE_FLOAT, -50, 100, NULL, NULL},
            {"Enable WiFi", "0", ITEM_TYPE_BOOL, 0, 1, NULL, NULL},
            {"HUAWEI", "3.14", ITEM_TYPE_FLOAT, 0, 100, NULL, NULL},
            {"Item10", "10", ITEM_TYPE_INT, 0, 100, NULL, NULL},
            {"Debug Mode", "0", ITEM_TYPE_BOOL, 0, 1, NULL, NULL},
            {"Item12", "12", ITEM_TYPE_INT, 0, 100, NULL, NULL},
            {"Item13", "13", ITEM_TYPE_INT, 0, 100, NULL, NULL},
            {"Item14", "14", ITEM_TYPE_INT, 0, 100, NULL, NULL},
            {"Item15", "15", ITEM_TYPE_INT, 0, 100, NULL, NULL}
        },
        .ItemCount = 15,
        .CurrentPage = 0,
        .CurrentSelection = 0
    };


    EnterMenu(&MainMenu); // 绘制主菜单
    printf("Menu Exited\n");
    // 此处编写用户代码 例如外设初始化代码等
    for(;;)
    {
        // 此处编写需要循环执行的代码
        

        


        // 此处编写需要循环执行的代码
    }
}

// **************************** 代码区域 ****************************
