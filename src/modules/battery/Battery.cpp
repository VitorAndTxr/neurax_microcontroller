#include "Battery.h"


Battery::Battery(int adc_input, double threshold)
{
    this->adc_input = adc_input;
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
    last_value = Adc::getValue(adc_input);
    return last_value;
}
