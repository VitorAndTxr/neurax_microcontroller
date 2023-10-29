#include "Battery.h"


Battery::Battery(Adc *adc, int adc_input, double threshold)
{
    this->adc_input = adc_input;
    this->adc = adc;
    this->threshold = threshold;
    last_value = 0;
}

Battery::~Battery()
{
}

bool Battery::isLow()
{
    return last_value < threshold;
}

double Battery::updateBatteryLevel()
{
    last_value = adc->getValue(adc_input);
    return last_value;
}
