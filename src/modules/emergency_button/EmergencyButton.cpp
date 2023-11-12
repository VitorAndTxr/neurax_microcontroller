#include "EmergencyButton.h"

void EmergencyButton::init() {
	attachInterrupt(
			PIN_EMERGENCY_BUTTON, 
			EmergencyButton::emergencyStop, 
			FALLING);
}

void EmergencyButton::stop() {
	detachInterrupt(PIN_EMERGENCY_BUTTON);
}

void EmergencyButton::emergencyStop() {
	Fes::emergency_stop = true;
	Session::pause();
}