#ifndef FES_MODULE
#define FES_MODULE

#include <Arduino.h>
#include "../potentiometer/Potentiometer.h"
#include "../../Globals.h"

struct FesParameters {
    static int fes_duration_ms;
    static int pulse_width_ms;
    static double frequency;
};
class Fes
{
private:
    static FesParameters parameters;
    static Potentiometer potentiometer;
    static TaskHandle_t fes_loop_handle;
    static bool status;

public:
    Fes() = delete;
    ~Fes() = delete;
    static void init();
    static void initGpio();
    static void setParameters(
        int fes_duration_ms = DEFAULT_STIMULI_DURATION, 
        int pulse_width_ms = DEFAULT_PULSE_WIDTH,
        double frequency = DEFAULT_FREQUENCY
        );
    static void setFesDurationMs(int fes_duration_ms = DEFAULT_STIMULI_DURATION);
    static void setPulseWidthMs(int fes_duration_ms = DEFAULT_PULSE_WIDTH);
    static void setFrequency(double frequency = DEFAULT_FREQUENCY);

    static void begin();
    static void increaseAmplitude(int steps = DEFAULT_POTENTIOMETER_STEPS);
    static void decreaseAmplitude(int steps = DEFAULT_POTENTIOMETER_STEPS);

    static void fesLoop(void *obj);
    static void fesLoopTaskWrapper(void *obj);
    static bool isOn();
};

#endif