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
	delay(1000); // Wait for serial to stabilize

	Serial.println("\n\n===========================================");
	Serial.println("=== FIRMWARE NEUROESTIMULATOR STARTED ===");
	Serial.println("===========================================\n");

	disableCore0WDT();
	disableCore1WDT();

	LED_POWER.set(true);
	Serial.println("[MAIN] LED Power initialized");


	// Enable logs for debugging (change to ESP_LOG_NONE to disable all logs)
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

// Test function for 215 Hz continuous mode streaming
void test215HzStreaming() {
	static bool initialized = false;
	static unsigned long sample_count = 0;
	static unsigned long last_stats_time = 0;

	// Initialize once
	if (!initialized) {
		Serial.println("\n===========================================");
		Serial.println("=== 215 Hz CONTINUOUS MODE TEST ===");
		Serial.println("===========================================\n");

		// Configure Butterworth filter for 215 Hz (same as streaming)
		float sampling_time_ms = 1000.0f / 215.0f;  // 4.65 ms
		SemgFilter::updateSamplingRate(sampling_time_ms, 10, 50, false);
		SemgFilter::resetState();

		Serial.println("[TEST] Filter configured: 215 Hz, 10-50 Hz bandpass + 60 Hz notch");

		// Start ADC continuous mode
		Adc::startContinuousMode(SEMG_ADC_PIN);
		Serial.println("[TEST] ADC continuous mode started");
		Serial.println("[TEST] Format: timestamp(ms), raw_adc, filtered_value");
		Serial.println("-------------------------------------------\n");

		initialized = true;
		last_stats_time = millis();
	}

	// Poll for new sample
	if (Adc::hasNewSample()) {
		// Get averaged sample (already downsampled 4x by ADC task)
		int16_t raw_sample = Adc::getLastSample();

		// Apply Butterworth filter (10-50 Hz bandpass + 60 Hz notch)
		float filtered = SemgFilter::filter((float)raw_sample);

		// Print: timestamp, raw, filtered
		//Serial.printf("Filtered:%.2f\n",  filtered);
		//Serial.printf("Time:%lu Raw: %d Filtered:%.2f\n", millis(), raw_sample, filtered);

		Serial.printf("%lu,%d,%.2f\n", millis(), raw_sample, filtered);

		sample_count++;
	}

	// Print statistics every 10 seconds
	if (millis() - last_stats_time > 10000) {
		Serial.printf("\n[STATS] Samples: %lu, Rate: %.1f Hz\n\n",
		              sample_count, (float)sample_count / 10.0f);
		sample_count = 0;
		last_stats_time = millis();
	}
}

void loop() {
	test215HzStreaming();
}





