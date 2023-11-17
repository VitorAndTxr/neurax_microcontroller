#include "Adc.h"

const int Adc::adc_i2c_address = ADC_I2C_ADDR; // already set in lib

bool Adc::error = false;
Adafruit_ADS1115 Adc::ads;

void Adc::init()
{
    Adc::ads.setDataRate(RATE_ADS1115_860SPS);

    if (!Adc::ads.begin()) {
        printDebug("Failed to initialize ADS1115.");
        Adc::error = true;
        while (1);
    }
}

float Adc::getValue(int input)
{
    return ads.computeVolts(ads.readADC_SingleEnded(input));
}