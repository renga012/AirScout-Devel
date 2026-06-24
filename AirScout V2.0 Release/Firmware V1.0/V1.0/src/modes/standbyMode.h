#ifndef MODES_SLEEPMODE_H
#define MODES_SLEEPMODE_H

#include "modes/baseMode.h"
class StandbyMode : public BaseMode {
public:
    bool init();
    void run();
    void exit();
};

#endif  // MODES_SLEEPMODE_H
