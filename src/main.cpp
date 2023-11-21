#include <Arduino.h>

#include "modules/adc/Adc.h"
#include "modules/fes/Fes.h"
#include "modules/semg/Semg.h"
#include "modules/message_handler/MessageHandler.h"
#include "modules/battery_monitor/Battery_monitor.h"
#include "modules/potentiometer/Potentiometer.h"
#include "modules/bluetooth/Bluetooth.h"
#include "modules/debug/Debug.h"
#include "modules/led/Led.h"




static const char* TAG_MAIN = "MAIN";
Led LED_POWER(LED_PIN_POWER);

void setup() {

	disableCore0WDT();
	disableCore1WDT();
	
	LED_POWER.set(true);
	Serial.begin(115200);
	
	esp_log_level_set("*", ESP_LOG_NONE);
	esp_log_system_timestamp();
	ESP_LOGI(TAG_MAIN, "Iniciando firmware NeuroEstimulator...");
	
	Gyroscope::init();
	Adc::init();
	
	Fes::init();
	Semg::init();
	Potentiometer::init();
	Session::init();
	MessageHandler::init();
	
	MessageHandler::start();
	

	//Led led_test(LED_PIN_TRIGGER);
	//led_test.set(true);
	//Led led_test2(LED_PIN_FES);
	//led_test2.set(true);
}	
float voltage = 2;
int direcao = 1;

void testeControleTensao(){
	
	float result;
	Serial.println("-----------------------------------inicio---------------------------");	
	Potentiometer::voltageSet(voltage);
	result = Potentiometer::getCorrectedVoltage();
	Serial.println("voltagem");	
	Serial.println(voltage);
	Serial.println("resultado");
	Serial.println(result);
	if (fabs(voltage-result)>0.4)
	{
		Serial.println("a diferença foi maior do que 0.4V");
		delay(1000000);
	}
	delay(1000);
	voltage += 0.4 * direcao;

    // Verifica se o valor atingiu 3.0 ou 0.0 para inverter a direção
    if (voltage >= 9.0 || voltage <= 2.0) {
        direcao *= -1; // Inverte a direção
    }
	
}

void testeControleTensao2(){
	
	float voltage2, result2;
	voltage2 = 7.0;
	Serial.println("-----------------------------------inicio---------------------------");	
	Potentiometer::voltageSet(voltage2);
	result2 = Potentiometer::getCorrectedVoltage();
	Serial.println("voltagem");	
	Serial.println(voltage2);
	Serial.println("resultado");
	Serial.println(result2);
	delay(3000);
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

void testeMedidasSEMG(){
	Serial.println(Semg::acquireAverage());
	if (Semg::isTrigger())
	{

	}
	delay(300);
}

void testesessao(){
	Session::detectionAndStimulation();
	delay(300);
}
void loop() {
	//Adc::getValue(1);
	//testRotinaGiroscopio();
	//testeMedidasSEMG();
	//testesessao();
	//testeControleTensao();
	

}





