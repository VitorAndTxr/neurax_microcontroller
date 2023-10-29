#include "Adc.h"

Adc::Adc()
{
    error = false;
    
    ads.setDataRate(RATE_ADS1115_860SPS);

    if (!ads.begin())
    {
        printDebug("Failed to initialize ADS1115.");
        error = true;
        while (1);
    }
}

Adc::~Adc()
{
}

inline int16_t Adc::getValue(int input)
{
    return ads.computeVolts(ads.readADC_SingleEnded(input));
}