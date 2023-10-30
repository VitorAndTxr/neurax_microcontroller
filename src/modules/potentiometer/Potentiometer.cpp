#include "Potentiometer.h"

Potentiometer::Potentiometer()
{
    pinMode(POTENTIOMETER_PIN_INCREMENT, OUTPUT);
    pinMode(POTENTIOMETER_PIN_UP_DOWN, OUTPUT);
    pinMode(POTENTIOMETER_PIN_CS, OUTPUT);

}

Potentiometer::~Potentiometer()
{
}

void Potentiometer::pulse(int steps) 
{
    for(int i = 0; i< steps; i++){
        digitalWrite(POTENTIOMETER_PIN_INCREMENT, HIGH);
        delayMicroseconds(10);
        digitalWrite(POTENTIOMETER_PIN_INCREMENT, LOW);
    }
}

inline void Potentiometer::increase(int steps) 
{
    digitalWrite(POTENTIOMETER_PIN_UP_DOWN, HIGH);
    digitalWrite(POTENTIOMETER_PIN_CS, LOW);
    
    pulse(steps);

    printDebug("Voltage up");
}


inline void Potentiometer::decrease(int steps) 
{
    digitalWrite(POTENTIOMETER_PIN_UP_DOWN, LOW);
    digitalWrite(POTENTIOMETER_PIN_CS, LOW);

    pulse(steps);

    printDebug("Voltage down");
}