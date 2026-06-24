#ifndef MAIN_H
#define MAIN_H
#include <vector>

#include "gui/elements/menu_bar.h"
#include "gui/elements/status_bar.h"
#include "gui/fonts/LiberationMono10x16.h"
#include "gui/fonts/LiberationMono13x21.h"
#include "gui/fonts/LiberationMono17x28.h"
#include "gui/fonts/LiberationMono8x13.h"
#include "settings.h"
#include "utils.h"

extern Settings user_settings;
extern Font_Liberation_Mono8x13 font_Liberation_Mono8x13;
extern Font_Liberation_Mono10x16 font_Liberation_Mono10x16;
extern Font_Liberation_Mono13x21 font_Liberation_Mono13x21;
extern Font_Liberation_Mono17x28 font_Liberation_Mono17x28;
// extern Font_Liberation_Mono21x35 font_Liberation_Mono21x35;
// extern Font_Liberation_Mono29x47 font_Liberation_Mono29x47;

extern GUI::Menu_Bar menu_bar;
extern GUI::Status_Bar status_bar;
extern modes_t modes;
extern std::vector<GUI::Element *> active_elements;
#endif  // MAIN_H
