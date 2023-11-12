#include "Fes.h"
#include "../debug/Debug.h"

int FesParameters::fes_duration_ms = DEFAULT_STIMULI_DURATION;
int FesParameters::pulse_width_ms = DEFAULT_PULSE_WIDTH;
float FesParameters::frequency = DEFAULT_FREQUENCY;
float FesParameters::amplitude = DEFAULT_AMPLITUDE;
bool Fes::emergency_stop = false;


TaskHandle_t Fes::fes_loop_handle = NULL;
bool Fes::status = false;

//bool emergency_stop = false;


inline void negativeHBridge(){
#if FES_MODULE_ENABLE
    digitalWrite(H_BRIDGE_INPUT_2, LOW);
    digitalWrite(H_BRIDGE_INPUT_1, HIGH);
#endif
}

inline void positiveHBridge(){
#if FES_MODULE_ENABLE
    digitalWrite(H_BRIDGE_INPUT_1, LOW);
    digitalWrite(H_BRIDGE_INPUT_2, HIGH);
#endif
}

inline void hBridgeReset(){
#if FES_MODULE_ENABLE
    digitalWrite(H_BRIDGE_INPUT_1, LOW);
    digitalWrite(H_BRIDGE_INPUT_2, LOW);
#endif
}

void Fes::init()
{
    Fes::initGpio();
    Fes::setParameters();
    hBridgeReset();
}

inline void Fes::initGpio()
{
#if FES_MODULE_ENABLE
    pinMode(H_BRIDGE_INPUT_1, OUTPUT);
    pinMode(H_BRIDGE_INPUT_2, OUTPUT);
#endif
}

void Fes::increaseAmplitude(int steps)
{
    potentiometer.increase();
}

void Fes::decreaseAmplitude(int steps)
{
    potentiometer.decrease();
}

void Fes::fesLoopTaskWrapper(void *obj) 
{
/*     // Cast the void pointer back to the class instance
    Fes *instance = static_cast<Fes *>(obj);

    // Call the member function using the instance
    instance->fesLoop(); */
}

void Fes::fesLoop(void *obj) 
{
#if FES_MODULE_ENABLE
    int single_pulse_duration_ms = Fes::parameters.pulse_width_ms / 2;
    int remaining_time = (1 / (Fes::parameters.frequency)) - Fes::parameters.pulse_width_ms;

    Fes::status = true;
    while (!Fes::emergency_stop)
    {
        positiveHBridge();
        vTaskDelay(single_pulse_duration_ms / portTICK_PERIOD_MS);

        negativeHBridge();
        vTaskDelay(single_pulse_duration_ms / portTICK_PERIOD_MS);

        hBridgeReset();
        delayMicroseconds(remaining_time);
#endif
    }
}

void Fes::begin()
{
    //criar uma task com pri max (lembrar de fazer a isr do botao de emergencia)
    
    xTaskCreatePinnedToCore( // Use xTaskCreate() in vanilla FreeRTOS
        &Fes::fesLoop,// Function to be called
        "FES Loop", // Name of task
        1024, // Stack size (bytes in ESP32, words in FreeRTOS)
        NULL, // Parameter to pass to function
        20, // Task priority (0 to configmAx_PRIORITIES - 1)
        &Fes::fes_loop_handle, // Task handle
        secondary_cpu) ;

    vTaskDelay(Fes::parameters.fes_duration_ms / portTICK_PERIOD_MS);
    // terminate the task after fes duration
    vTaskDelete(Fes::fes_loop_handle);
    Fes::fes_loop_handle = NULL; 

    Fes::status = false;
}


void Fes::setParameters(int fes_duration_ms, int pulse_width_ms, double frequency) {
    Fes::parameters.fes_duration_ms = fes_duration_ms;
    Fes::parameters.pulse_width_ms = pulse_width_ms;
    Fes::parameters.frequency = frequency;
}

void Fes::setFesDurationMs(int fes_duration_ms) {
    Fes::parameters.fes_duration_ms = fes_duration_ms;
}

void Fes::setPulseWidthMs(int pulse_width_ms) {
    Fes::parameters.pulse_width_ms = pulse_width_ms;
}

void Fes::setFrequency(double frequency) {
    Fes::parameters.frequency = frequency;
}

bool Fes::isOn() {
    return Fes::status;
}
