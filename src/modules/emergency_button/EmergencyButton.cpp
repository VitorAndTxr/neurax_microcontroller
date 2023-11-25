#include "EmergencyButton.h"

void EmergencyButton::init() {
	ESP_LOGI(TAG_EMR, "Setting up interrupt on pin %d [MODE: RISING]...", PIN_EMERGENCY_BUTTON);
	pinMode(PIN_EMERGENCY_BUTTON, INPUT);
	
	int i = digitalRead(PIN_EMERGENCY_BUTTON);
	ESP_LOGI(TAG_EMR, "reading interrupt pin %d ", i);
	attachInterrupt(
			PIN_EMERGENCY_BUTTON, 
			EmergencyButton::emergencyStop, 
			RISING);
}

void EmergencyButton::stop() {
	detachInterrupt(PIN_EMERGENCY_BUTTON);
	ESP_LOGI(TAG_EMR, "Disabledinterrupt on pin %d", PIN_EMERGENCY_BUTTON);
}

void IRAM_ATTR EmergencyButton::emergencyStop() {
	Fes::stimulating = false;
	Fes::emergency_stop = true;
	digitalWrite(H_BRIDGE_INPUT_1, LOW);
    digitalWrite(H_BRIDGE_INPUT_2, LOW);
	if (Fes::isOn) {
	 	Session::status.interrupted_stimuli_amount++;
	}
	
	// Fes::emergency_stop = true;
	// if (Session::task_handle != NULL) {
	// 	//ESP_LOGD(TAG_SESSION, "Session task suspended.");
	// 	vTaskSuspend(Session::task_handle);	
	// }
	// Session::status.paused = true;
	// if (Fes::isOn) {
	// 	Fes::stimulating = false;
	// 	Session::status.interrupted_stimuli_amount++;
	// }
	// digitalWrite(H_BRIDGE_INPUT_1, LOW);
    // digitalWrite(H_BRIDGE_INPUT_2, LOW);


	//Session::pauseFromMessageHandler();
	//ESP_LOGW(TAG_EMR, "===== Emergency stop =====");
}