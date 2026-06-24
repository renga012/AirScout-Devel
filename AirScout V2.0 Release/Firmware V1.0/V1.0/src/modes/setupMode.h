#ifndef MODES_SETUPMODE_H
#define MODES_SETUPMODE_H
#include "modes/baseMode.h"

class SetupMode : public BaseMode {
public:
    SetupMode();
    bool init();
    void run();
    void exit();
};

#endif  // MODES_SETUPMODE_H
