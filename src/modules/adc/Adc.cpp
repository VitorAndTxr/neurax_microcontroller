#include "Adc.h"

const int Adc::adc_i2c_address = ADC_I2C_ADDR; // already set in lib

bool Adc::error = false;
Adafruit_ADS1115 Adc::ads;

void Adc::init() {
	ESP_LOGI(TAG_ADC, "Setup...");

#if ADC_MODULE_ENABLE
    Adc::ads.setDataRate(RATE_ADS1115_860SPS);

    if (!Adc::ads.begin()) {
		ESP_LOGE(TAG_ADC, "Failed to initialize ADS1115.");

        Adc::error = true;
        while (1) {};
    }
#else
	ESP_LOGW(TAG_ADC, "ADC module is not enabled.");
#endif
}

float Adc::getValue(int input) {
#if ADC_MODULE_ENABLE
	//ESP_LOGD(TAG_ADC, "Reading input %d", input);
	float value = ads.computeVolts(ads.readADC_SingleEnded(input));
	//ESP_LOGD(TAG_ADC, "Value: %lf", value);
    return value;
#else
	ESP_LOGW(TAG_ADC, "ADC module is not enabled. Returning 0.0f.");
    return 0.0f;
#endif
}