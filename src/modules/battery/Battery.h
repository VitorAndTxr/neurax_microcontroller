#ifndef BATTERY_MODULE
#define BATTERY_MODULE

#include "../adc/Adc.h"

class Battery
{
private:
    Adc* adc;
    int adc_input;
    double last_value;
    double threshold;
public:
    Battery(Adc *adc, int adc_input, double threshold = 3.0);
    ~Battery();
    bool isLow();
    double updateBatteryLevel();
};

#endif