#include "EmergencyButton.h"

void EmergencyButton::init() {
	ESP_LOGI(TAG_EMR, "Setting up interrupt on pin %d [MODE: RISING]...", PIN_EMERGENCY_BUTTON);
	
	attachInterrupt(
			PIN_EMERGENCY_BUTTON, 
			EmergencyButton::emergencyStop, 
			RISING);
}

void EmergencyButton::stop() {
	detachInterrupt(PIN_EMERGENCY_BUTTON);
	ESP_LOGI(TAG_EMR, "Disabledinterrupt on pin %d", PIN_EMERGENCY_BUTTON);
}

void EmergencyButton::emergencyStop() {
	Fes::emergency_stop = true;
	Session::pause();
	ESP_LOGW(TAG_EMR, "===== Emergency stop =====");
}