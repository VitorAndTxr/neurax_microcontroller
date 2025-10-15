#ifndef ADC_MODULE
#define ADC_MODULE
#include <Adafruit_ADS1X15.h>
#include "../debug/Debug.h"
#include "../../globals.h"

static const char* TAG_ADC = "ADC";

// Continuous mode configuration
#define ADC_DOWNSAMPLE_RATIO 4
#define ADC_FIXED_RATE_HZ 215  // 860 Hz ÷ 4 = 215 Hz
#define ADC_CIRCULAR_BUFFER_SIZE 512  // Buffer for 2.4 seconds @ 215 Hz

class Adc
{
private:
    static const int adc_i2c_address;
    static Adafruit_ADS1115 ads;
    static bool error;

    // Continuous mode variables
    static int16_t downsample_buffer[ADC_DOWNSAMPLE_RATIO];
    static volatile int downsample_index;

    // Circular buffer for 215 Hz output
    static int16_t circular_buffer[ADC_CIRCULAR_BUFFER_SIZE];
    static volatile int write_index;
    static volatile int read_index;
    static volatile int available_samples;

    static TaskHandle_t adc_task_handle;

    // Private methods
    static void adcTaskLoop(void* parameters);

public:
    Adc() = delete;
    ~Adc() = delete;

    // Initialization
    static void init();

    // Legacy interface (kept for backward compatibility with Session/FES)
    static float getValue(int input);

    // NEW: Continuous mode interface (for streaming)
    static void startContinuousMode(int channel);
    static void stopContinuousMode();
    static bool hasNewSample();
    static int16_t getLastSample();
};

#endif