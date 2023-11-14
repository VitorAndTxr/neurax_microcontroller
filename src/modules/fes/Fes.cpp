#include "Fes.h"
#include "../debug/Debug.h"

int FesParameters::fes_duration_ms = DEFAULT_STIMULI_DURATION;
int FesParameters::pulse_width_ms = DEFAULT_PULSE_WIDTH;
float FesParameters::frequency = DEFAULT_FREQUENCY;
float FesParameters::amplitude = DEFAULT_AMPLITUDE;
bool Fes::emergency_stop = false;
TimerHandle_t Fes::fesTimer = NULL;


TaskHandle_t Fes::fes_loop_handle = NULL;
volatile bool Fes::stimulating = false;

void negativeHBridge(){
#if FES_MODULE_ENABLE
    digitalWrite(H_BRIDGE_INPUT_2, LOW);
    digitalWrite(H_BRIDGE_INPUT_1, HIGH);
#endif
}

void positiveHBridge(){
#if FES_MODULE_ENABLE
    digitalWrite(H_BRIDGE_INPUT_1, LOW);
    digitalWrite(H_BRIDGE_INPUT_2, HIGH);
#endif
}

void Fes::hBridgeReset(){
#if FES_MODULE_ENABLE
    digitalWrite(H_BRIDGE_INPUT_1, LOW);
    digitalWrite(H_BRIDGE_INPUT_2, LOW);
#endif
}

void Fes::init()
{
    Fes::initGpio();
    Fes::setParameters();
    Fes::hBridgeReset();
}

void Fes::initGpio()
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

void Fes::fesLoop() {
#if FES_MODULE_ENABLE
    int single_pulse_duration_ms = Fes::parameters.pulse_width_ms / 2;
    int remaining_time = (1 / (Fes::parameters.frequency)) - Fes::parameters.pulse_width_ms;

    Fes::stimulating = true;
    while (!Fes::emergency_stop && stimulating)
    {
        positiveHBridge();
        vTaskDelay(single_pulse_duration_ms / portTICK_PERIOD_MS);

        negativeHBridge();
        vTaskDelay(single_pulse_duration_ms / portTICK_PERIOD_MS);

        Fes::hBridgeReset();
        delayMicroseconds(remaining_time);
    }
#endif
}

void Fes::begin() {
    fesTimer = xTimerCreate(
        "FES timer",           // Nome do temporizador (para fins de depuração)
        pdMS_TO_TICKS(Fes::parameters.fes_duration_ms),  // Período em milissegundos
        pdFALSE,              // Modo autoreload, o temporizador será recarregado automaticamente
        (void *)0,           // ID do temporizador (pode ser usado para identificação adicional)
        Fes::stopFes        // Função a ser chamada quando o temporizador expirar
    );

    // Verificação se o temporizador foi criado com sucesso
    if (fesTimer != NULL) {
        xTimerStart(fesTimer, 0);
        Fes::fesLoop();
    } else {
        printDebug("Erro ao criar o temporizador do FES!");
    }
}

void Fes::stopFes(void * parameters) {
    Fes::stimulating = false;
}
void Fes::stopFes() {
    Fes::stimulating = false;
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
    return Fes::stimulating;
}
