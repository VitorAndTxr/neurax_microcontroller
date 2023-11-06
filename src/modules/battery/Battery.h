#ifndef BATTERY_MODULE
#define BATTERY_MODULE

#include "../adc/Adc.h"
#include "../adc/Adc.h"


class Battery
{
private:
    int adc_input;
    double last_value;
    double threshold;
public:
    Battery(int adc_input, double threshold = DEFAULT_BATTERY_THRESHOLD);
    ~Battery();
    bool isLow();
    double updateBatteryLevel();
};

#endif