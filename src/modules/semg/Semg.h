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
	static void stopSamplingTimer();
    static void startLedTrigger();
    static void createLedTriggerTimer();
	static void setDifficulty(int difficulty);
	static void sensorTask(void * obj); 
    
};

#endif