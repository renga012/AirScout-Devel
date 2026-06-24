#ifndef DRIVER_BASESENSOR_H
#define DRIVER_BASESENSOR_H

enum class Sensor_State { OK, FAIL, DISABLED };

class BaseSensor {
public:
    BaseSensor() = default;
    // BaseSensor();
    // ~BaseSensor();
    bool init();
    bool deinit(); 
    void getValues();
    Sensor_State state;
};

#endif  // DRIVER_BASESENSOR_H
