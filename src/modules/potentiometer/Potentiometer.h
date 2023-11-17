#ifndef POTENTIOMETER_MODULE
#define POTENTIOMETER_MODULE

#include <Arduino.h>
#include "../debug/Debug.h"

static const char* TAG_POT = "POT";

class Potentiometer
{
private:


public:
    Potentiometer() = delete;
    ~Potentiometer() = delete;
    static void init();
    static void pulse(int steps = 1);
    static void increase(int steps = 1);
    static void decrease(int steps = 1);
    static float voltageSet(float voltage);
    static float getCorrectedVoltage();
};

#endif