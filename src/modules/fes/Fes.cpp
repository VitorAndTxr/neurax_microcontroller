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

Led LED_FES(LED_PIN_FES);

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
	ESP_LOGI(TAG_FES, "Setup...");
    Fes::initGpio();
    Fes::setParameters();
    Fes::hBridgeReset();
	ESP_LOGI(TAG_FES, "Setup done.");
}

void Fes::initGpio()
{
#if FES_MODULE_ENABLE
    pinMode(H_BRIDGE_INPUT_1, OUTPUT);
    pinMode(H_BRIDGE_INPUT_2, OUTPUT);
#endif
}

void Fes::increaseAmplitude(int steps) {
    potentiometer.increase();
}

void Fes::decreaseAmplitude(int steps) {
    potentiometer.decrease();
}

void Fes::fesLoop() {
#if FES_MODULE_ENABLE
    int single_pulse_duration_ms = Fes::parameters.pulse_width_ms / 2;
    int remaining_time = (1 / (Fes::parameters.frequency)) - Fes::parameters.pulse_width_ms;

	Potentiometer::voltageSet(Fes::parameters.amplitude);
	ESP_LOGI(TAG_FES, "Starting stimulation");
	LED_FES.set(true);
    Fes::stimulating = true;
	// TODO emergency stop
    unsigned long startTime = millis();
    while (!Fes::emergency_stop && millis() - startTime < 2000 ) {//stimulating
        //Serial.println("estimulando 3 ---------");
        positiveHBridge();
        delayMicroseconds(400);

        negativeHBridge();
        delayMicroseconds(400);

        Fes::hBridgeReset();
        delay(16);
    }
	ESP_LOGI(TAG_FES, "Stoped stimulation");
	LED_FES.set(false);
#endif
}

void Fes::begin() {
	ESP_LOGI(TAG_FES, "Creating timer...");
    fesTimer = xTimerCreate(
        "FES timer",           // Nome do temporizador (para fins de depuração)
        pdMS_TO_TICKS(5000),  // Período em milissegundos
        pdFALSE,              // Modo autoreload, o temporizador será recarregado automaticamente
        (void *)0,           // ID do temporizador (pode ser usado para identificação adicional)
        Fes::stopFes        // Função a ser chamada quando o temporizador expirar
    );
    // Verificação se o temporizador foi criado com sucesso
    if (fesTimer != NULL) {
		ESP_LOGI(TAG_FES, "Starting timer");
        xTimerStart(fesTimer, 0);
        Fes::fesLoop();
    } else {
		ESP_LOGE(TAG_FES, "Error creating timer!");
    }
}

void Fes::stopFes(void * parameters) {
    Fes::stimulating = false;
	ESP_LOGI(TAG_FES, "Timer completed");
	ESP_LOGI(TAG_FES, "Stopping FES timer...");
}
/*
void Fes::stopFes() {
    Fes::stimulating = false;
	ESP_LOGI(TAG_FES, "Stopping FES...");
}
*/
void Fes::setParameters(int fes_duration_ms, int pulse_width_ms, double frequency) {
	ESP_LOGI(TAG_FES, "Setting parameters");
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
