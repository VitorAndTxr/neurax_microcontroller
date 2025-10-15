#include "Adc.h"

const int Adc::adc_i2c_address = ADC_I2C_ADDR; // already set in lib

bool Adc::error = false;
Adafruit_ADS1115 Adc::ads;

// Continuous mode variables
int16_t Adc::downsample_buffer[ADC_DOWNSAMPLE_RATIO] = {0};
volatile int Adc::downsample_index = 0;

// Circular buffer for 215 Hz output
int16_t Adc::circular_buffer[ADC_CIRCULAR_BUFFER_SIZE] = {0};
volatile int Adc::write_index = 0;
volatile int Adc::read_index = 0;
volatile int Adc::available_samples = 0;

TaskHandle_t Adc::adc_task_handle = NULL;

void Adc::init() {
	Serial.println("[ADC] Initializing ADS1115...");
	ESP_LOGI(TAG_ADC, "Initializing ADS1115...");

#if ADC_MODULE_ENABLE
    if (!Adc::ads.begin()) {
		Serial.println("[ADC] ERROR: Failed to initialize ADS1115!");
		ESP_LOGE(TAG_ADC, "Failed to initialize ADS1115");
        Adc::error = true;
        while (1) {};
    }

    Serial.println("[ADC] ADS1115 begin() succeeded");

    // IMPORTANT: Set data rate AFTER begin() to prevent reset
    Adc::ads.setDataRate(RATE_ADS1115_860SPS);
    Serial.printf("[ADC] setDataRate(RATE_ADS1115_860SPS) called (value: %d)\n", RATE_ADS1115_860SPS);

    // Verify by reading back (if library supports it)
    uint16_t config = Adc::ads.getGain(); // Just to test I2C works
    Serial.printf("[ADC] I2C communication test - gain read: %d\n", config);

	Serial.println("[ADC] ADS1115 configured for 860 SPS");
	ESP_LOGI(TAG_ADC, "ADS1115 initialized @ 860 SPS");
#else
	ESP_LOGW(TAG_ADC, "ADC module disabled (ADC_MODULE_ENABLE=false)");
#endif
}

// Legacy interface - kept for backward compatibility with Session/FES
float Adc::getValue(int input) {
#if ADC_MODULE_ENABLE
    float value = ads.computeVolts(ads.readADC_SingleEnded(input));
    return value;
#else
	ESP_LOGW(TAG_ADC, "ADC module disabled. Returning 0.0f");
    return 0.0f;
#endif
}

// NEW: Start continuous mode with downsample
void Adc::startContinuousMode(int channel) {
#if ADC_MODULE_ENABLE
	Serial.printf("[ADC] Starting continuous mode on channel %d (860 Hz → %d Hz)\n",
             channel, ADC_FIXED_RATE_HZ);
	ESP_LOGI(TAG_ADC, "Starting continuous mode on channel %d (860 Hz → %d Hz)",
             channel, ADC_FIXED_RATE_HZ);

    // Reset downsample buffer
    downsample_index = 0;

    // Reset circular buffer
    write_index = 0;
    read_index = 0;
    available_samples = 0;
    Serial.printf("[ADC] Circular buffer reset: write=%d, read=%d, available=%d\n",
                  write_index, read_index, available_samples);

    // Create ADC task on Core 1 (communication core)
    // Pass channel as parameter
    BaseType_t result = xTaskCreatePinnedToCore(
        Adc::adcTaskLoop,
        "ADC Continuous",
        4096,                  // Stack size
        (void*)(long)channel,  // Pass channel number to task
        18,                    // Priority (higher than streaming=15)
        &adc_task_handle,
        1                      // Core 1
    );

    if (result == pdPASS) {
		Serial.println("[ADC] Task created successfully");
		ESP_LOGI(TAG_ADC, "Continuous mode started successfully");
    } else {
		Serial.printf("[ADC] ERROR: Failed to create task (error: %d)\n", result);
		ESP_LOGE(TAG_ADC, "Failed to create ADC task (error: %d)", result);
    }
#else
	Serial.println("[ADC] WARNING: ADC module disabled");
	ESP_LOGW(TAG_ADC, "ADC module disabled");
#endif
}

// NEW: Stop continuous mode
void Adc::stopContinuousMode() {
#if ADC_MODULE_ENABLE
	ESP_LOGI(TAG_ADC, "Stopping continuous mode...");

    if (adc_task_handle != NULL) {
        vTaskDelete(adc_task_handle);
        adc_task_handle = NULL;
		ESP_LOGI(TAG_ADC, "ADC task deleted");
    }

    // Reset circular buffer state
    write_index = 0;
    read_index = 0;
    available_samples = 0;
#endif
}

// NEW: Check if samples available in circular buffer
bool Adc::hasNewSample() {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);
    bool has_sample = (available_samples > 0);
    portEXIT_CRITICAL(&mux);
    return has_sample;
}

// NEW: Get sample from circular buffer (215 Hz output)
int16_t Adc::getLastSample() {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);

    // Read from circular buffer
    int16_t sample = 0;
    if (available_samples > 0) {
        sample = circular_buffer[read_index];
        read_index = (read_index + 1) % ADC_CIRCULAR_BUFFER_SIZE;
        available_samples--;
    }

    portEXIT_CRITICAL(&mux);
    return sample;
}

// NEW: ADC continuous sampling task
void Adc::adcTaskLoop(void* parameters) {
#if ADC_MODULE_ENABLE
	int channel = (int)(long)parameters;
	Serial.printf("[ADC] Task loop started! Channel=%d, Downsample=%d:1\n", channel, ADC_DOWNSAMPLE_RATIO);
	ESP_LOGI(TAG_ADC, "ADC task loop started (downsample %d:1)", ADC_DOWNSAMPLE_RATIO);

    unsigned long samples_collected = 0;
    unsigned long last_log_time = millis();
    int samples_written_to_buffer = 0;

    // Timing measurement
    unsigned long total_read_time_us = 0;
    unsigned long min_read_time_us = 999999;
    unsigned long max_read_time_us = 0;

    // Start continuous conversion mode (only once)
    Serial.printf("[ADC] Starting continuous conversion on channel %d\n", channel);
    ads.startADCReading(channel, false);  // false = continuous mode
    delay(2);  // Wait for first conversion

    while (true) {
        // Measure ADC read time
        unsigned long read_start = micros();

        // Read last conversion result (non-blocking, just reads register)
        int16_t raw_value = ads.getLastConversionResults();

        unsigned long read_duration = micros() - read_start;

        // Track timing stats
        total_read_time_us += read_duration;
        if (read_duration < min_read_time_us) min_read_time_us = read_duration;
        if (read_duration > max_read_time_us) max_read_time_us = read_duration;

        // Delay to match 860 Hz rate (1163 us per sample)
        // Subtract actual I2C read time to compensate for overhead
        int delay_us = 1163 - read_duration;
        if (delay_us > 0) {
            delayMicroseconds(delay_us);
        }

        // Add to downsample buffer
        downsample_buffer[downsample_index++] = raw_value;
        samples_collected++;

        // When buffer full, compute average (860Hz ÷ 4 = 215Hz)
        if (downsample_index >= ADC_DOWNSAMPLE_RATIO) {
            // Average 4 samples for anti-aliasing
            int32_t sum = 0;
            for (int i = 0; i < ADC_DOWNSAMPLE_RATIO; i++) {
                sum += downsample_buffer[i];
            }
            int16_t averaged = sum / ADC_DOWNSAMPLE_RATIO;

            // Write to circular buffer atomically
            portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
            portENTER_CRITICAL(&mux);

            // Only write if buffer not full
            if (available_samples < ADC_CIRCULAR_BUFFER_SIZE) {
                circular_buffer[write_index] = averaged;
                write_index = (write_index + 1) % ADC_CIRCULAR_BUFFER_SIZE;
                available_samples++;
                samples_written_to_buffer++;
            }
            // Note: If buffer full, sample is dropped (overflow protection)

            portEXIT_CRITICAL(&mux);

            // Debug: Print first 5 samples
            if (samples_written_to_buffer <= 5) {
                Serial.printf("[ADC] Sample #%d written: %d (buffer: %d/%d)\n",
                              samples_written_to_buffer, averaged, available_samples, ADC_CIRCULAR_BUFFER_SIZE);
            }

            // Reset downsample buffer
            downsample_index = 0;
        }

        // Periodic stats logging (every 10 seconds)
        if (millis() - last_log_time > 10000) {
            portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
            portENTER_CRITICAL(&mux);
            int buffered = available_samples;
            portEXIT_CRITICAL(&mux);

            // Calculate timing stats
            unsigned long avg_read_time_us = (samples_collected > 0) ?
                (total_read_time_us / samples_collected) : 0;

            Serial.printf("[ADC] Stats: %lu samples/10s (%lu Hz), buffer: %d/%d\n",
                         samples_collected, samples_collected / 10, buffered, ADC_CIRCULAR_BUFFER_SIZE);
            Serial.printf("[ADC] Timing: avg=%lu us, min=%lu us, max=%lu us per read\n",
                         avg_read_time_us, min_read_time_us, max_read_time_us);
            Serial.printf("[ADC] Expected @ 860 SPS: 1163 us/sample\n");

            ESP_LOGI(TAG_ADC, "ADC stats: %lu samples/10s (avg rate: %lu Hz), buffer: %d/%d",
                     samples_collected, samples_collected / 10, buffered, ADC_CIRCULAR_BUFFER_SIZE);

            samples_collected = 0;
            total_read_time_us = 0;
            min_read_time_us = 999999;
            max_read_time_us = 0;
            last_log_time = millis();
        }

        // No delay - run as fast as ADC allows (~860 Hz)
    }
#endif
}