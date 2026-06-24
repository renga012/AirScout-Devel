#ifndef MODES_SETTINGSMODE_H
#define MODES_SETTINGSMODE_H

#include "gui/elements/selection.h"
#include "modes/baseMode.h"
class SettingsMode : public BaseMode {
public:
    bool init() override;
    void run() override;
    void exit() override;
    private:
    GUI::Selection m_selection;
};

#endif  // MODES_SETTINGSMODE_H
