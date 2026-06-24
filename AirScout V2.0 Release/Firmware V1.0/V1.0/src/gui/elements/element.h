#ifndef GUI_ELEMENTS_ELEMENT_H
#define GUI_ELEMENTS_ELEMENT_H

#include "driver/display.h"
namespace GUI {
class Element {
public:
    Element() = default;
    Element(Display *disp);

    // force (bool): When True, the element is completly redrawn. When False, only when the content was changed, the content is redrawn
    virtual bool update(bool force);
    virtual void drawStatic();
    virtual void drawContent();
    bool redraw = true;

protected:
    bool m_changed;
    Display *m_disp;
};
}  // namespace GUI
#endif  // GUI_ELEMENTS_ELEMENT_H
