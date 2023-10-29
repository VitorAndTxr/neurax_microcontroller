#include "Fes.h"
#include "../debug/Debug.h"

Fes::Fes()
{
    this->initGpio();
    this->setParameters();
    fes_loop_handle = NULL;
}

Fes::~Fes()
{
}

inline void Fes::initGpio()
{
    pinMode(H_BRIDGE_INPUT_1, OUTPUT);
    pinMode(H_BRIDGE_INPUT_2, OUTPUT);
}

void Fes::increaseAmplitude(int steps)
{
    potentiometer.increase();
}

void Fes::decreaseAmplitude(int steps)
{
    potentiometer.decrease();
}

inline void negativeHBridge(){
    digitalWrite(H_BRIDGE_INPUT_2, LOW);
    digitalWrite(H_BRIDGE_INPUT_1, HIGH);
}

inline void positiveHBridge(){
    digitalWrite(H_BRIDGE_INPUT_1, LOW);
    digitalWrite(H_BRIDGE_INPUT_2, HIGH);
}
inline void hBridgeReset(){
    digitalWrite(H_BRIDGE_INPUT_1, LOW);
    digitalWrite(H_BRIDGE_INPUT_2, LOW);
}

void Fes::fesLoopTaskWrapper(void *obj) 
{
    // Cast the void pointer back to the class instance
    Fes *instance = static_cast<Fes *>(obj);

    // Call the member function using the instance
    instance->fesLoop();
}

void Fes::fesLoop() 
{
    int single_pulse_duration_ms = Fes::pulse_width_ms / 2;
    int remaining_time = (1 / (Fes::frequency)) - Fes::pulse_width_ms;
    while (true)
    {
        positiveHBridge();
        vTaskDelay(single_pulse_duration_ms / portTICK_PERIOD_MS);

        negativeHBridge();
        vTaskDelay(single_pulse_duration_ms / portTICK_PERIOD_MS);

        hBridgeReset();
        delayMicroseconds(remaining_time);
    }
}

inline void Fes::begin()
{
    //criar uma task com pri max (lembrar de fazer a isr do botao de emergencia)
    
    xTaskCreatePinnedToCore( // Use xTaskCreate() in vanilla FreeRTOS
        &fesLoopTaskWrapper,// Function to be called
        "FES Loop", // Name of task
        1024, // Stack size (bytes in ESP32, words in FreeRTOS)
        this, // Parameter to pass to function
        20, // Task priority (0 to configmAx_PRIORITIES - 1)
        &this->fes_loop_handle, // Task handle
        main_cpu) ;

    vTaskDelay(Fes::fes_duration_ms / portTICK_PERIOD_MS);
    // terminate the task after fes duration
    vTaskDelete(this->fes_loop_handle);
    this->fes_loop_handle = NULL; 
}


void Fes::setParameters(int fes_duration_ms, int pulse_width_ms, double frequency) {
    Fes::fes_duration_ms = fes_duration_ms;
    Fes::pulse_width_ms = pulse_width_ms;
    Fes::frequency = frequency;
}

void Fes::setFesDurationMs(int fes_duration_ms) {
    Fes::fes_duration_ms = fes_duration_ms;
}

void Fes::setPulseWidthMs(int pulse_width_ms) {
    Fes::pulse_width_ms = pulse_width_ms;
}

void Fes::setFrequency(double frequency) {
    Fes::frequency = frequency;
}