#include <Arduino.h>

#include "modules/adc/Adc.h"
#include "modules/fes/Fes.h"
#include "modules/semg/Semg.h"
#include "modules/message_handler/MessageHandler.h"
#include "modules/battery_monitor/Battery_monitor.h"
#include "modules/potentiometer/Potentiometer.h"
#include "modules/bluetooth/Bluetooth.h"
#include "modules/debug/Debug.h"

static const char* TAG_MAIN = "MAIN";

void setup() {
	Serial.begin(115200);
	
	esp_log_level_set("*", ESP_LOG_DEBUG);
	esp_log_system_timestamp();
	ESP_LOGI(TAG_MAIN, "Iniciando firmware NeuroEstimulator...");

	//Adc::init();
	/*
	Fes::init();
	Semg::init();*/
	Potentiometer::init();
	/*Session::init();
	Gyroscope::init();
	//MessageHandler::init();
	Gyroscope::init();
	
	Serial.begin(115200);
	Gyroscope::calibrateMPU6050;
	ESP_LOGI(TAG_MAIN, "Setup concluido.");
	Semg::updateTriggerThreshold();

	//MessageHandler::start();
	*/
}	
float voltage = 0.1;
int direcao = 1;

void testeControleTensao(){
	/*
	for (int i = 0; i<=20 ; i++ ){
		Serial.println(Potentiometer::getCorrectedVoltage());
		Potentiometer::increase(1);
		delay (2000);
		Serial.println(Potentiometer::getCorrectedVoltage());
		
	}
	
	for (int i = 0; i<=20; i++ ){
		Serial.println(Potentiometer::getCorrectedVoltage());
		Potentiometer::decrease(1);
		delay (2000);
		Serial.println(Potentiometer::getCorrectedVoltage());
		
	}*/
	
	float result;
	Serial.println("-----------------------------------inicio---------------------------");	
	Potentiometer::voltageSet(4 * voltage);
	result = Potentiometer::getCorrectedVoltage();
	Serial.println("voltagem");	
	Serial.println(voltage * 4);
	Serial.println("resultado");
	Serial.println(result);
	if (fabs(voltage-result)>1)
	{
		delay(1000000);
	}
	delay(1000);
	voltage += 0.1 * direcao;

    // Verifica se o valor atingiu 3.0 ou 0.0 para inverter a direção
    if (voltage >= 3.0 || voltage <= 0.0) {
        direcao *= -1; // Inverte a direção
    }
	
	//MessageHandler::loop();
	//BatteryMonitor::loop();
}

void testSEMG(){
	Serial.println("teste-----------------------------------------------------------------");

	Semg::acquireAverage();

	bool triggered = false;
	while (!triggered)
	{
		Serial.println("teste-----------------------------------------------------------------");

		triggered = Semg::isTrigger();

		if (triggered) {
			Serial.println("amem");
		}
	}
	

}
void testMedidasGiroscopio(){
	Serial.println(Gyroscope::calculatePitch());
	delay(500);
}


void testRotinaGiroscopio(){
	Serial.println("comecando uma rotina");
	Serial.println(Gyroscope::gyroscopeRoutine());
	Serial.println("fim da rotina");
	delay(500);
}

void testTrigger(){
	if(Serial.available()){
    char c = Serial.read();

		if(c=='u')
		{
		testSEMG();  
		}
	}
	//Serial.println(Semg::acquireAverage());
	ESP_LOGI(TAG_MAIN, "%f",Semg::acquireAverage() );
	delay(200);

}


void testeControleTensao1(){
	for (int i = 0; i<=20; i++){
		Potentiometer::increase(1);
		//Potentiometer::getCorrectedVoltage();
		delay(1000);
	}
	for (int i = 0; i<=20; i++){
		Potentiometer::decrease(1);
		//Potentiometer::getCorrectedVoltage();
		delay(1000);

	}
}
void loop() {
	//Adc::getValue(1);
	testeControleTensao1();
}





