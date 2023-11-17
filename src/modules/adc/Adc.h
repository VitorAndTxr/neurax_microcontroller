#ifndef ADC_MODULE
#define ADC_MODULE
#include <Adafruit_ADS1X15.h>
#include "../debug/Debug.h"
#include "../../globals.h"

static const char* TAG_ADC = "ADC";

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
    static float getValue(int input);
};

#endif