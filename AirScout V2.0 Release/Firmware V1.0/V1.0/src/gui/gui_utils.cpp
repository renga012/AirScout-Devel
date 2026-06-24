#include "gui_utils.h"

#include "driver/init_hardware.h"
#include "globals.h"
#include "gui/elements/menu_bar.h"
#include "gui/elements/status_bar.h"
#include "main.h"
namespace GUI {

void clear_except_status_menu(Display::COLOR c) {
    disp.fill_rectangle(0, STATUS_BAR_HEIGHT + 1, 320, MENU_BAR_BEGIN - STATUS_BAR_HEIGHT - 1, c);
}
void activateElement(GUI::Element *element) {
    active_elements.push_back(element);
    element->redraw = true;
}
void resetActiveElements(){
    active_elements.resize(ALWAYS_ACTIVE_ELEMENTS);
}
}  // namespace GUI