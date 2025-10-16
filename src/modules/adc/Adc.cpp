#include "Adc.h"

const int Adc::adc_i2c_address = ADC_I2C_ADDR; // already set in lib

bool Adc::error = false;
Adafruit_ADS1115 Adc::ads;

// Continuous mode variables (averaging buffer)
int16_t Adc::downsample_buffer[ADC_DOWNSAMPLE_RATIO] = {0};
volatile int Adc::downsample_index = 0;

// Circular buffer for 215 Hz output
int16_t Adc::circular_buffer[ADC_CIRCULAR_BUFFER_SIZE] = {0};
volatile int Adc::write_index = 0;
volatile int Adc::read_index = 0;
volatile int Adc::available_samples = 0;

TaskHandle_t Adc::adc_task_handle = NULL;

void Adc::init() {
#if ADC_MODULE_ENABLE
    if (!Adc::ads.begin()) {
		ESP_LOGE(TAG_ADC, "Failed to initialize ADS1115");
        Adc::error = true;
        while (1) {};
    }

    // IMPORTANT: Set data rate AFTER begin() to prevent reset
    Adc::ads.setDataRate(RATE_ADS1115_860SPS);
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

// NEW: Start continuous mode with decimation (1 of 4 samples)
void Adc::startContinuousMode(int channel) {
#if ADC_MODULE_ENABLE
    // Reset decimation counter
    downsample_index = 0;

    // Reset circular buffer
    write_index = 0;
    read_index = 0;
    available_samples = 0;

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

    if (result != pdPASS) {
		ESP_LOGE(TAG_ADC, "Failed to create ADC task (error: %d)", result);
    }
#else
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

    unsigned long samples_collected = 0;
    unsigned long last_log_time = millis();
    int samples_written_to_buffer = 0;

    // Timing measurement
    unsigned long total_read_time_us = 0;
    unsigned long min_read_time_us = 999999;
    unsigned long max_read_time_us = 0;

    // Convert channel number (0-3) to MUX config for single-ended mode
    // ADS1X15_REG_CONFIG_MUX_SINGLE_0 = 0x4000, _1 = 0x5000, _2 = 0x6000, _3 = 0x7000
    uint16_t mux_config = 0x4000 + (channel << 12);  // 0x4000 + (channel * 0x1000)

    // Start continuous conversion mode in single-ended mode
    ads.startADCReading(mux_config, true);  // true = continuous mode
    delay(2);  // Wait for first conversion

    ESP_LOGI(TAG_ADC, "Continuous mode started on channel %d", channel);

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

        // Downsample: average 4 samples (860Hz ÷ 4 = 215Hz)
        downsample_buffer[downsample_index] = raw_value;
        downsample_index++;
        samples_collected++;

        if (downsample_index >= ADC_DOWNSAMPLE_RATIO) {
            // Calculate average of 4 samples (anti-aliasing filter)
            int32_t sum = 0;
            for (int i = 0; i < ADC_DOWNSAMPLE_RATIO; i++) {
                sum += downsample_buffer[i];
            }
            int16_t averaged_value = sum / ADC_DOWNSAMPLE_RATIO;

            // Write to circular buffer atomically
            portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
            portENTER_CRITICAL(&mux);

            // Only write if buffer not full
            if (available_samples < ADC_CIRCULAR_BUFFER_SIZE) {
                circular_buffer[write_index] = averaged_value;
                write_index = (write_index + 1) % ADC_CIRCULAR_BUFFER_SIZE;
                available_samples++;
                samples_written_to_buffer++;
            }
            // Note: If buffer full, sample is dropped (overflow protection)

            portEXIT_CRITICAL(&mux);

            // Reset downsample counter
            downsample_index = 0;
        }

        // Periodic stats logging removed for clean serial output

        // No delay - run as fast as ADC allows (~860 Hz)
    }
#endif
}