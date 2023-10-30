#ifndef SEMG_MODULE
#define SEMG_MODULE
#include "../SemgFilter/SemgFilter.h"
#include "../adc/Adc.h"

class Semg
{
private:
    
    static float filtered_value[SEMG_SAMPLES_PER_VALUE];
    static float mes[5];
    static float output;

    static float gain;
    static float difficulty;

    static float threshold;

    static float mes_a[2];
    static float mes_b[2];
    static float voltage;
public:
    Semg() = delete;
    ~Semg() = delete;
    static void init();
    static void increaseDifficulty(
        int increment = SEMG_DIFFICULTY_INCREMENT);
    static void decreaseDifficulty(
        int decrement = SEMG_DIFFICULTY_INCREMENT);
    static bool isTrigger();
    static void updateMovingThreshold();
    static bool impedanceTooLow();
    static float readSensor();
    static float getFilteredSample();
    static float acquireAverage(int readings_amount = 2);
};

#endif