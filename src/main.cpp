#include <Arduino.h>

#include "modules/adc/Adc.h"
#include "modules/fes/Fes.h"
#include "modules/semg/Semg.h"
#include "modules/bluetooth/Bluetooth.h"
#include "modules/message_handler/MessageHandler.h"
#include "modules/battery_monitor/Battery_monitor.h"
#include "modules/potentiometer/Potentiometer.h"
#include "modules/debug/Debug.h"
void setup() {
	Serial.begin(115200);
	printDebug("Iniciando esp");
	//Adc::init();
	//Fes::init();
	//Semg::init();
	//Potentiometer::init();
	//Battery_monitor::init();
	//Bluetooth::init();
	MessageHandler::init();
}
float voltage = 0.5;
void loop() {
	MessageHandler::start();
	//BatteryMonitor::loop();
}




