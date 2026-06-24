#ifndef MODES_MODE_H
#define MODES_MODE_H

enum class MODES { Drive, Idle, Upload, Standby, Mode, WifiSetup, Setup, ModeSelection, Settings};

class BaseMode {
public:
    virtual bool init() {return true;};
    virtual void run() {};
    virtual void exit() {};
};

#endif  // MODES_MODE_H
