#ifndef SEMG_MODULE
#define SEMG_MODULE
#include "../SemgFilter/SemgFilter.h"
#include "../adc/Adc.h"
#include "../fes/Fes.h"
#include <ArduinoJson.h>

#include "../debug/Debug.h"
#include "../message_handler/MessageHandler.h"
#include "../../globals.h"
#include "../led/Led.h"

static const char* TAG_SEMG = "sEMG";

struct SemgParameters {
    static float gain;
    static float difficulty;
    static float threshold;
};

// Streaming data types
enum StreamingDataType {
    STREAMING_RAW,      // Raw ADC values
    STREAMING_FILTERED, // Butterworth filtered
    STREAMING_RMS       // RMS envelope
};

// Streaming configuration
struct StreamingConfig {
    int rate;                      // Hz - samples per second
    StreamingDataType type;        // Data type to stream
    int samples_per_packet;        // Number of samples per message
    int packets_per_second;        // Calculated from rate and samples_per_packet
};
class Semg
{
private:
    static float filtered_value[SEMG_SAMPLES_PER_VALUE];
    volatile static float raw_value[SEMG_SAMPLES_PER_VALUE];
    static float mes[5];

    static float output;

    static float mes_a[2];
    static float mes_b[2];
    static volatile float voltage;
	static void disableSensor();
	static void enableSensor();
    static float readSensor();
    static bool outputIsInInterval(float lower_limit, float higher_limit);
    static float getFilteredSample();
	static void filterSamplesArray();
	static const float sampling_period_ms;

    // Streaming private members
    static float streaming_buffer[STREAMING_BUFFER_SIZE];
    static volatile int buffer_write_index;
    static volatile int buffer_read_index;
    static volatile bool streaming_active;
    static StreamingConfig streaming_config;
    static TaskHandle_t streaming_task_handle;
    static unsigned long streaming_start_time;

    static void writeToBuffer(float value);
    static float applyStreamingFilter(float value);
    static bool sendStreamingMessage(float* samples, int count);

public:
	static volatile int sample_amount;
    Semg() = delete;
    ~Semg() = delete;
    static void init();
    static void increaseDifficulty(
        int increment = SEMG_DIFFICULTY_INCREMENT);
    static void decreaseDifficulty(
        int decrement = SEMG_DIFFICULTY_INCREMENT);
    static bool isTrigger();
    static void updateTriggerThreshold();
    static bool impedanceTooLow();
    static float acquireAverage(int readings_amount = SEMG_DEFAULT_READINGS_AMOUNT);
    static SemgParameters parameters;
	static void sendTriggerMessage();
	static void testTrigger(int test_duration = SEMG_DEFAULT_TEST_DURATION_SECONDS);
	static void samplingCallback(TimerHandle_t xTimer);
    static void ledTriggerCallback(TimerHandle_t xTimer);
	static TimerHandle_t samplingTimer;
	static TimerHandle_t ledTriggerTimer;
    static TaskHandle_t task_handle;
	static void startSamplingTimer();
    static void startStreamingSamplingTimer(float period_ms);
	static void stopSamplingTimer();
    static void startLedTrigger();
    static void createLedTriggerTimer();
	static void setDifficulty(int difficulty);
	static void sensorTask(void * obj);

    // Streaming public API
    static void configureStreaming(int rate, const char* type_str);
    static void enableStreaming();
    static void disableStreaming();
    static bool isStreaming();
    static void streamingTask(void* parameters);
    static int getAvailableSamples();
    static void readStreamingSamples(float* output, int count);

};

#endif