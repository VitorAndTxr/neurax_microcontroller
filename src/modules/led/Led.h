#ifndef LED_MODULE
#define LED_MODULE

#include <Arduino.h>

class Led
{
private:
    int pin;
    bool status;
	//TimerHandle_t timer;
public:
    Led(int pin);
    ~Led();
    void set(bool on = false);
    void toggle();
    void blink(int amount = 1, int interval = 50);
	void turnOnFor(int ms);
	void setOn();
	static void setTriggerLedOff(void * obj);
};

#endif