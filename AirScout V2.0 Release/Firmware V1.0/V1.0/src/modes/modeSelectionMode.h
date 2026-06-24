#ifndef MODES_SELECTIONMODE_H
#define MODES_SELECTIONMODE_H
#include "gui/elements/selection.h"
#include "modes/baseMode.h"
class ModeSelectionMode : public BaseMode {
public:
    bool init() override;
    void run() override;
    void exit() override;

private:
    GUI::Selection m_selection;
};

#endif  // MODES_SELECTIONMODE_H
