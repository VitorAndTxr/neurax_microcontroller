#ifndef POTENTIOMETER_MODULE
#define POTENTIOMETER_MODULE

#include <Arduino.h>
#include "../debug/Debug.h"

class Potentiometer
{
private:


public:


    Potentiometer();
    ~Potentiometer();
    static void init();
    static void pulse(int steps = 1);
    static void increase(int steps = 1);
    static void decrease(int steps = 1);
    static float voltageSet(float voltage);
    static float getCorrectedVoltage();
};

#endif