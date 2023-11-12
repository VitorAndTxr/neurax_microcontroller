#ifndef EMERGENCY_BUTTON_H
#define EMERGENCY_BUTTON_H
#include <Arduino.h>
#include "../fes/Fes.h"
#include "../session/Session.h"

class EmergencyButton
{
private:
public:
	EmergencyButton() = delete;
	~EmergencyButton() = delete;
	static void init();
	static void stop();
	static void emergencyStop();
};


#endif
