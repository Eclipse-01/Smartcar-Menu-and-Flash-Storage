#ifndef ITEMS_H
#define ITEMS_H

// 前向声明
typedef struct Menu Menu;

typedef enum {
    ITEM_TYPE_INT,
    ITEM_TYPE_FLOAT,
    ITEM_TYPE_BOOL,
    ITEM_TYPE_MENU,
    ITEM_TYPE_FUNCTION
} ItemType;

typedef struct{
    char ItemName[20]; // 菜单项名称
    char ItemValue[10]; // 菜单项值
    ItemType Type; // 菜单项类型
    float MinValue; // 最小值（仅对整数和浮点数有效）
    float MaxValue; // 最大值（仅对整数和浮点数有效）
    Menu *SubMenu; // 子菜单指针（仅对菜单类型有效）
    void (*ItemFunction)(void); // 函数指针（仅对函数类型有效）
}MenuItem;

typedef struct Menu{
    char MenuTitle[20]; // 菜单名称
    MenuItem Items[64]; // 菜单项数组
    int ItemCount; // 菜单项数量
    int CurrentPage; // 当前页码
    int CurrentSelection; // 当前选中项
}Menu;

#endif
