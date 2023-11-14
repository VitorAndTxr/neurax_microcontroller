#ifndef SEMG_MODULE
#define SEMG_MODULE
#include "../SemgFilter/SemgFilter.h"
#include "../adc/Adc.h"
#include "../fes/Fes.h"
#include <ArduinoJson.h>

#include "../debug/Debug.h"
#include "../message_handler/MessageHandler.h"
#include "../../globals.h"

struct SemgParameters {
    static float gain;
    static float difficulty;
    static float threshold;
};
class Semg
{
private:
    
    static float filtered_value[SEMG_SAMPLES_PER_VALUE];
    static float mes[5];

    static float output;

    static float mes_a[2];
    static float mes_b[2];
    static float voltage;
	static void disableSensor();
	static void enableSensor();
    static float readSensor();
    static bool outputIsInInterval(float lower_limit, float higher_limit);
    static float getFilteredSample();

public:
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
};

#endif