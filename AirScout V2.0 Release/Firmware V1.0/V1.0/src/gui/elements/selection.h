#ifndef GUI_ELEMENTS_SELECTION_H
#define GUI_ELEMENTS_SELECTION_H
#include <stdint.h>

#include "driver/display.h"
#include "globals.h"
#include "gui/elements/element.h"
#include "gui/elements/status_bar.h"
#include "modes/baseMode.h"
namespace GUI {

struct _selection_item;
typedef struct _selection_item selection_item;

class Selection : public GUI::Element {
public:
    enum class TASKS { SCROLL, NUMBER, ACTIVATE, ACTIVATE_MODE, TOGGLE };
    enum class POS : uint16_t { TEXT_BEGIN_X = 40, TEXT_BEGIN_Y = STATUS_BAR_HEIGHT + 20, FONT_HEIGHT = 21, ITEM_HEIGHT = FONT_HEIGHT + 10 };
    // enum class TYPE { TOGGLE, NUMBER, ACTIVATE_MODE, ACTIVATE };
    Selection(Display *disp, selection_item *items, const int16_t item_cnt);
    ~Selection();  // names
    Selection();   // names

    void drawStatic();
    void drawContent();
    bool update(bool force);
    // returns selected field
    void processBTN(BTN btn);
    bool should_exit();

private:
    bool m_draw_item(selection_item *m_items, uint16_t cnt);
    void m_draw_selection_icon(selection_item *m_item, uint16_t cnt);
    void m_draw_items(uint16_t offset);

    void m_draw_toggle_menu(bool force);
    void m_draw_number_menu(bool force);

    void m_handle_BTN1();
    void m_handle_BTN2();
    void m_handle_BTN3();
    void m_handle_BTN4();

    selection_item *m_get_selected_item();

    TASKS m_current_task;
    int16_t m_item_ctn;
    selection_item *m_items;

    int16_t m_currently_selected = 0;
    int16_t m_offset = 0;
    bool m_shared_content_changed = 0;
    bool m_force_redraw = 0;
    bool m_should_exit;
    static constexpr uint8_t m_items_per_page = 11;
    static constexpr Display::COLOR m_background_color = Display::COLOR::DODGERBLUE;
    static constexpr Display::COLOR m_text_color = Display::COLOR::WHITESMOKE;
};

typedef struct _selection_item {
    const char *name;
    Selection::TASKS type;
    void (*cbk)(selection_item *);
    union {
        bool toggle;
        struct {
            int32_t min;
            int32_t max;
            int32_t num;
            int32_t step_size;
            const char *unit;
        } number;
        MODES mode;
    } context;
} selection_item;

}  // namespace GUI
#endif  // GUI_ELEMENTS_SELECTION_H
