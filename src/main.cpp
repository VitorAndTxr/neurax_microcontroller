#include <Arduino.h>

#include "modules/adc/Adc.h"
#include "modules/fes/Fes.h"
#include "modules/semg/Semg.h"
#include "modules/message_handler/MessageHandler.h"
#include "modules/battery_monitor/Battery_monitor.h"
#include "modules/potentiometer/Potentiometer.h"
#include "modules/bluetooth/Bluetooth.h"
#include "modules/debug/Debug.h"

void setup() {
	Serial.begin(115200);
	printDebug("Iniciando esp");

	Adc::init();
	Fes::init();
	Semg::init();
	Potentiometer::init();
	Session::init();
	Gyroscope::init();
	MessageHandler::init();
	MessageHandler::start();
}
float voltage = 0.5;
void loop() {

	float result;
	
	result = Potentiometer::voltageSet(voltage);
	Serial.println("voltagem");	
	Serial.println(voltage);
	Serial.println("resultado");
	Serial.println(result);
	voltage = voltage+0.5;
	delay(5000);
	if ( voltage >=3){
		voltage = 0.5;
	}
	//MessageHandler::loop();
	//BatteryMonitor::loop();
}




