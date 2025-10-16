#include <Arduino.h>

#include "modules/adc/Adc.h"
#include "modules/fes/Fes.h"
#include "modules/semg/Semg.h"
#include "modules/SemgFilter/SemgFilter.h"
#include "modules/message_handler/MessageHandler.h"
#include "modules/battery_monitor/Battery_monitor.h"
#include "modules/potentiometer/Potentiometer.h"
#include "modules/bluetooth/Bluetooth.h"
#include "modules/debug/Debug.h"
#include "modules/led/Led.h"

static const char* TAG_MAIN = "MAIN";
Led LED_POWER(LED_PIN_POWER);

void setup() {
	Serial.begin(115200);
	delay(1000);

	Serial.println("\n\n===========================================");
	Serial.println("=== FIRMWARE NEUROESTIMULATOR STARTED ===");
	Serial.println("===========================================\n");

	disableCore0WDT();
	disableCore1WDT();

	LED_POWER.set(true);
	Serial.println("[MAIN] LED Power initialized");

	esp_log_level_set("*", ESP_LOG_INFO);
	esp_log_system_timestamp();
	ESP_LOGI(TAG_MAIN, "Iniciando firmware NeuroEstimulator...");

	Serial.println("[MAIN] Initializing Gyroscope...");
	Gyroscope::init();

	Serial.println("[MAIN] Initializing ADC...");
	Adc::init();

	Serial.println("[MAIN] Initializing FES...");
	Fes::init();

	Serial.println("[MAIN] Initializing sEMG...");
	Semg::init();

	Serial.println("[MAIN] Initializing Potentiometer...");
	Potentiometer::init();

	Serial.println("[MAIN] Initializing Session...");
	Session::init();

	Serial.println("[MAIN] Initializing MessageHandler...");
	MessageHandler::init();

	Serial.println("[MAIN] Starting MessageHandler task...");
	MessageHandler::start();

	Serial.println("\n===========================================");
	Serial.println("=== FIRMWARE INITIALIZATION COMPLETE ===");
	Serial.println("===========================================\n");
}

void loop() {
	vTaskDelay(1000);
}





