#ifndef ADC_MODULE
#define ADC_MODULE
#include <Adafruit_ADS1X15.h>
#include "../debug/debug.h"

class Adc
{
private:
    
    const int adc_i2c_address = ADC_I2C_ADDR; // already set in lib
    
    Adafruit_ADS1115 ads;
    bool error;
public:
    Adc();
    ~Adc();
    int16_t getValue(int input);
};

#endif