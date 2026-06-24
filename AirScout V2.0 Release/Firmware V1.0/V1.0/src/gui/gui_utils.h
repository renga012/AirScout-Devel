#ifndef GUI_GUI_UTILS_H
#define GUI_GUI_UTILS_H

#include "driver/display.h"
#include "gui/elements/element.h"
namespace GUI {
void clear_except_status_menu(Display::COLOR c);

void activateElement(GUI::Element *element);
void resetActiveElements();
}  // namespace GUI

#endif  // GUI_GUI_UTILS_H
