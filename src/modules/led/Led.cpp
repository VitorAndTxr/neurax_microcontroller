#include "Led.h"
#include <Arduino.h>


Led::Led(int pin)
{
    this->pin = pin;
    pinMode(pin, OUTPUT);

    this->set(false);
}

Led::~Led()
{
}

void Led::set(bool on)
{
    digitalWrite(this->pin, on);
    this->status = on;
}

void Led::toggle() 
{
    this->status = !this->status;
    this->set(this->status);
}

void Led::blink(int amount, int interval) 
{
    for (int i = 0; i < amount; i++) {
        this->set(true);
        delay(interval);
        this->set(false);
        delay(interval);
    }    
}
