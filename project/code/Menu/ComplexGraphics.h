#ifndef COMPLEXGRAPHICS_H
#define COMPLEXGRAPHICS_H

#include "Items.h"
#include "BasicGeos.h"
#include "zf_common_headfile.h"

void DrawMenu(const Menu* menu);
void SetValueComponent_Int(Menu* menu, int item_index);
void SetValueComponent_Float(Menu* menu, int item_index);
void SetValueComponent_Bool(Menu* menu, int item_index);
void EnterSubMenu(Menu* menu, int item_index);
void ExecuteFunction(Menu* menu, int item_index);

#endif
