#ifndef EMERGENCY_BUTTON_H
#define EMERGENCY_BUTTON_H
#include <Arduino.h>
#include "../fes/Fes.h"
#include "../session/Session.h"

static const char* TAG_EMR = "EMR";

class EmergencyButton
{
private:
public:
	EmergencyButton() = delete;
	~EmergencyButton() = delete;
	static void init();
	static void stop();
	static void IRAM_ATTR emergencyStop();
};


#endif
