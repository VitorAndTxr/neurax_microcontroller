#ifndef FES_MODULE
#define FES_MODULE

#include <Arduino.h>
#include "../potentiometer/Potentiometer.h"
#include "../../Globals.h"

class Fes
{
private:
    static int fes_duration_ms;
    static int pulse_width_ms;
    static double frequency;
    static Potentiometer potentiometer;
    static TaskHandle_t fes_loop_handle;

public:
    Fes();
    ~Fes();
    void initGpio();
    void setParameters(
        int fes_duration_ms = DEFAULT_STIMULI_DURATION, 
        int pulse_width_ms = DEFAULT_PULSE_WIDTH,
        double frequency = DEFAULT_FREQUENCY
        );
    void setFesDurationMs(int fes_duration_ms = DEFAULT_STIMULI_DURATION);
    void setPulseWidthMs(int fes_duration_ms = DEFAULT_PULSE_WIDTH);
    void setFrequency(double frequency = DEFAULT_FREQUENCY);

    void begin();
    void increaseAmplitude(int steps = DEFAULT_POTENTIOMETER_STEPS);
    void decreaseAmplitude(int steps = DEFAULT_POTENTIOMETER_STEPS);

    void fesLoop();
    static void fesLoopTaskWrapper(void *obj);
};

#endif