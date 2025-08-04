#include "Items.h"
#include <string.h>

// --- Forward declarations for custom functions ---
extern void SystemReset(void);

// --- Menu struct definitions ---
Menu MainMenu;
Menu AppleJuice;

// --- Menu initialization functions ---
// --- Initialization for MainMenu ---
void Init_MainMenu(void) {
    strcpy(MainMenu.MenuTitle, "MainMenu");
    MainMenu.ItemCount = 4;
    MainMenu.CurrentPage = 0;
    MainMenu.CurrentSelection = 0;

    // Item: HUAWEI
    strcpy(MainMenu.Items[0].ItemName, "HUAWEI");
    MainMenu.Items[0].Type = ITEM_TYPE_INT;
    strcpy(MainMenu.Items[0].ItemValue, "190");

    // Item: Xiaomi
    strcpy(MainMenu.Items[1].ItemName, "Xiaomi");
    MainMenu.Items[1].Type = ITEM_TYPE_FLOAT;
    strcpy(MainMenu.Items[1].ItemValue, "114.514");

    // Item: Samsung
    strcpy(MainMenu.Items[2].ItemName, "Samsung");
    MainMenu.Items[2].Type = ITEM_TYPE_BOOL;
    strcpy(MainMenu.Items[2].ItemValue, "1");

    // Item: Apple
    strcpy(MainMenu.Items[3].ItemName, "Apple");
    MainMenu.Items[3].Type = ITEM_TYPE_MENU;
    MainMenu.Items[3].SubMenu = &AppleJuice;

}

// --- Initialization for AppleJuice ---
void Init_AppleJuice(void) {
    strcpy(AppleJuice.MenuTitle, "AppleJuice");
    AppleJuice.ItemCount = 1;
    AppleJuice.CurrentPage = 0;
    AppleJuice.CurrentSelection = 0;

    // Item: Function
    strcpy(AppleJuice.Items[0].ItemName, "Function");
    AppleJuice.Items[0].Type = ITEM_TYPE_FUNCTION;
    AppleJuice.Items[0].ItemFunction = &SystemReset;

}

// --- Main initialization function ---
void InitializeAllMenus(void) {
    Init_MainMenu();
    Init_AppleJuice();
}