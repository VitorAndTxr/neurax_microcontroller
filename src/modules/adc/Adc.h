#ifndef ADC_MODULE
#define ADC_MODULE
#include <Adafruit_ADS1X15.h>
#include "../debug/debug.h"

class Adc
{
private:
    static const int adc_i2c_address; 
    static Adafruit_ADS1115 ads;
    static bool error;
public:
    Adc() = delete;
    ~Adc() = delete;
    static void init();
    static int16_t getValue(int input);
};

#endif