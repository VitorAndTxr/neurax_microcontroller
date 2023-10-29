#include "Potentiometer.h"

Potentiometer::Potentiometer()
{
    pinMode(INCREMENT_POTENTIOMETER, OUTPUT);
    pinMode(UP_DOWN_POTENTIOMETER, OUTPUT);
    pinMode(CS_POTENTIOMETER, OUTPUT);

}

Potentiometer::~Potentiometer()
{
}

void Potentiometer::pulse(int steps) 
{
    for(int i = 0; i< steps; i++){
        digitalWrite(INCREMENT_POTENTIOMETER, HIGH);
        delayMicroseconds(10);
        digitalWrite(INCREMENT_POTENTIOMETER, LOW);
    }
}

inline void Potentiometer::increase(int steps) 
{
    digitalWrite(UP_DOWN_POTENTIOMETER, HIGH);
    digitalWrite(CS_POTENTIOMETER, LOW);
    
    pulse(steps);

    printDebug("Voltage up");
}


inline void Potentiometer::decrease(int steps) 
{
    digitalWrite(UP_DOWN_POTENTIOMETER, LOW);
    digitalWrite(CS_POTENTIOMETER, LOW);

    pulse(steps);

    printDebug("Voltage down");
}