#ifndef FES_MODULE
#define FES_MODULE

#include <Arduino.h>
#include "../potentiometer/Potentiometer.h"
#include "../../globals.h"

struct FesParameters {
    static int fes_duration_ms;
    static int pulse_width_ms;
    static float frequency;
    static float amplitude;
};
class Fes
{
private:
    static Potentiometer potentiometer;
    static TaskHandle_t fes_loop_handle;
    static bool stimulating;
    static TimerHandle_t fesTimer;

public:
    Fes() = delete;
    ~Fes() = delete;
    static void init();
    static void initGpio();
    static void hBridgeReset();
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

    static void fesLoop();
    static void fesLoopTaskWrapper(void *obj);
    static bool isOn();
    static void stopFes(void * parameters);
    static FesParameters parameters;
	static bool emergency_stop;
};

#endif