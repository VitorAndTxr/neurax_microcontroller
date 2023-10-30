#include "Semg.h"

float Semg::filtered_value[SEMG_SAMPLES_PER_VALUE] = {0};
float Semg::mes[5] = {0};
float Semg::gain = SEMG_DEFAULT_GAIN;
float Semg::difficulty = SEMG_DIFFICULTY_DEFAULT;
float Semg::threshold = 0;
float Semg::mes_a[2] = {0};
float Semg::mes_b[2] = {0};
float Semg::voltage = 0.0f;

void Semg::init() {
    pinMode(SEMG_ENABLE_PIN, OUTPUT);
    digitalWrite(SEMG_ENABLE_PIN, LOW);
}

void Semg::increaseDifficulty(int increment) {
    Semg::difficulty += increment;
    Semg::difficulty = (Semg::difficulty > SEMG_DIFFICULTY_MAXIMUM) ? SEMG_DIFFICULTY_MAXIMUM : Semg::difficulty;
}

void Semg::decreaseDifficulty(int decrement) {
    Semg::difficulty -= decrement;
    Semg::difficulty = (Semg::difficulty < SEMG_DIFFICULTY_MINIMUM) ? SEMG_DIFFICULTY_MINIMUM : Semg::difficulty;
}

bool Semg::isTrigger() {
    return false;
}

void Semg::updateMovingThreshold() {
    
}

bool Semg::impedanceTooLow() {
    return (Semg::output > SEMG_LOW_IMPEDANCE_THRESHOLD) ? true : false;
}

float Semg::getFilteredSample() {
    for (int i = 0; i < SEMG_SAMPLES_PER_VALUE; i++) {
        Semg::readSensor();
        Semg::filtered_value[i] = SemgFilter::filter(Semg::voltage);

        delay(1);
    }

    float average_every_n_samples;
    float sum_of_averages = 0;

    for (int i = 0; i < SEMG_SAMPLES_PER_VALUE/SEMG_SAMPLES_PER_AVERAGE; i++)
    {
        average_every_n_samples = 0;
        for (int j = 0; j < SEMG_SAMPLES_PER_AVERAGE; j++)
        {
            average_every_n_samples += Semg::filtered_value[SEMG_SAMPLES_PER_AVERAGE*i + j];
        }
        average_every_n_samples /= SEMG_SAMPLES_PER_AVERAGE;
        sum_of_averages += average_every_n_samples;
    }

    return (Semg::gain * sum_of_averages) / 10.0f;
}

float Semg::readSensor() {
    Semg::voltage = Adc::getValue(SEMG_ADC_PIN);
    return Semg::voltage;
}

float Semg::acquireAverage(int readings_amount) {
    for (int i = 0; i < readings_amount; i++)
    {
        Semg::output += Semg::getFilteredSample();
    }
    Semg::output /= readings_amount;
    
    return Semg::output;
}


