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

// Streaming task (igual ao código antigo de Semg.cpp)
// Roda em task dedicada com prioridade baixa (não bloqueia MessageHandler)
void streaming215HzTask(void* parameter) {
	unsigned long sample_count = 0;
	unsigned long last_stats_time = millis();
	unsigned long last_check_time = millis();
	unsigned long samples_received = 0;

	// Wait 1 second for ADC to stabilize
	vTaskDelay(1000);

	Serial.println("\n[TASK] Streaming task started");
	Serial.println("[TASK] Checking if ADC buffer has samples...\n");

	while (true) {
		// Poll for new sample (non-blocking)
		if (Adc::hasNewSample()) {
			samples_received++;

			// Get decimated sample (1 of 4 samples @ 860 Hz = 215 Hz output)
			int16_t raw_sample = Adc::getLastSample();

			// Apply Butterworth filter (10-50 Hz bandpass)
			float filtered = SemgFilter::filterWithNotch((float)raw_sample);

			sample_count++;

			// Debug: Print first 10 samples to diagnose
			if (sample_count <= 10) {
				Serial.printf("[DEBUG #%lu] Raw ADC: %d | Filtered: %.2f\n",
				              sample_count, raw_sample, filtered);
			}

			// After warm-up period (50 samples = 230ms), start printing
			if (sample_count == 51) {
				Serial.println("\n[TASK] Warm-up complete - starting continuous stream\n");
			}

			if (sample_count > 50) {
				Serial.printf("%.2f\n", filtered);
			}

			// Print statistics every 5 seconds
			if (sample_count > 50 && (millis() - last_stats_time > 5000)) {
				unsigned long samples_printed = sample_count - 50;
				float actual_rate = (float)samples_printed / ((millis() - (last_stats_time - 5000)) / 1000.0f);
				Serial.printf("\n[STATS] Samples: %lu, Rate: %.1f Hz\n\n",
				              samples_printed, actual_rate);
				last_stats_time = millis();
			}
		}

		// Check if samples are arriving (every 2 seconds)
		if (millis() - last_check_time > 2000) {
			if (samples_received == 0) {
				Serial.println("[ERROR] No samples received from ADC buffer!");
				Serial.println("[ERROR] Check if ADC task is running and writing to buffer");
			} else {
				Serial.printf("[INFO] Received %lu samples in last 2 seconds (%.1f Hz)\n",
				              samples_received, (float)samples_received / 2.0f);
			}
			samples_received = 0;
			last_check_time = millis();
		}

		// Yield to higher priority tasks (MessageHandler pode preemptar)
		vTaskDelay(1);  // 1ms delay
	}
}

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

	// DIAGNOSTIC: Test ADC with single-shot readings BEFORE continuous mode
	Serial.println("\n[DIAG] Testing ADC single-shot mode on all channels...");
	for (int ch = 0; ch < 4; ch++) {
		float voltage = Adc::getValue(ch);
		Serial.printf("  Channel %d: %.4f V (raw: %d)\n", ch, voltage, (int)(voltage / 0.0001875f));
	}
	Serial.println("[DIAG] ADC single-shot test complete\n");

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
	Serial.println("===========================================");

	// Configure Butterworth filter for 215 Hz
	float sampling_time_ms = 1000.0f / 215.0f;  // 4.651 ms
	SemgFilter::updateSamplingRate(sampling_time_ms, 10, 50, false);
	SemgFilter::resetState();

	// Start ADC continuous mode
	Adc::startContinuousMode(SEMG_ADC_PIN);

	// Create streaming task (SAME as old Semg streaming task)
	xTaskCreatePinnedToCore(
		streaming215HzTask,
		"Stream215Hz",
		4096,
		NULL,
		10,  // Priority 10 (BELOW MessageHandler=20, can be preempted)
		NULL,
		1    // Core 1 (communication core)
	);
}

void loop() {
	// Loop vazio - todo trabalho é feito em tasks dedicadas
	// Isso libera o Core 1 para MessageHandler e outras tasks
	vTaskDelay(1000);  // Sleep 1 segundo
}





