#ifndef LED_MODULE
#define LED_MODULE

class Led
{
private:
    int pin;
    bool status;

public:
    Led(int pin);
    ~Led();
    void set(bool on = false);
    void toggle();
    void blink(int amount = 1, int interval = 50);
};

#endif