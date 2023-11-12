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
    void pulse(int steps = 1);
    void increase(int steps = 1);
    void decrease(int steps = 1);
};

#endif