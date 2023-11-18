#include "Semg.h"

float Semg::filtered_value[SEMG_SAMPLES_PER_VALUE] = {0};
float Semg::mes[5] = {0};
float SemgParameters::gain = SEMG_DEFAULT_GAIN;
float SemgParameters::difficulty = 50;
//float SemgParameters::difficulty = SEMG_DIFFICULTY_DEFAULT;

float SemgParameters::threshold = 0;
float Semg::mes_a[2] = {0};
float Semg::mes_b[2] = {0};
volatile float Semg::voltage = 0.0f;
float Semg::output = 0.0f;
volatile int Semg::sample_amount = 0;
TimerHandle_t Semg::samplingTimer = NULL;
const float Semg::sampling_period_ms = SEMG_SAMPLING_PERIOD;

void Semg::init() {
    pinMode(SEMG_ENABLE_PIN, OUTPUT);
    Semg::disableSensor();
}

void Semg::increaseDifficulty(int increment) {
    Semg::parameters.difficulty += increment;
    Semg::parameters.difficulty = (Semg::parameters.difficulty > SEMG_DIFFICULTY_MAXIMUM) ? SEMG_DIFFICULTY_MAXIMUM : Semg::parameters.difficulty;

    Semg::updateTriggerThreshold();
}

void Semg::decreaseDifficulty(int decrement) {
    Semg::parameters.difficulty -= decrement;
    Semg::parameters.difficulty = (Semg::parameters.difficulty < SEMG_DIFFICULTY_MINIMUM) ? SEMG_DIFFICULTY_MINIMUM : Semg::parameters.difficulty;

    Semg::updateTriggerThreshold();
}

inline bool Semg::outputIsInInterval(float lower_limit, float higher_limit){
	ESP_LOGI(TAG_FES, "Verify Semg %f lower limit: %f", Semg::output, lower_limit);
    
    return Semg::output >= Semg::parameters.threshold && Semg::output <= SEMG_TRIGGER_THRESHOLD_MAXIMUM;
}

bool Semg::isTrigger() {
    return (outputIsInInterval(Semg::parameters.threshold, SEMG_TRIGGER_THRESHOLD_MAXIMUM) 
        && !Fes::isOn());
}

void Semg::updateTriggerThreshold() {
    Semg::parameters.threshold = (1 + (Semg::parameters.difficulty / 100)); //* Semg::mes[0]; 
    bool is_valid_low_threshold = Semg::parameters.threshold > SEMG_TRIGGER_THRESHOLD_MINIMUM;
    Semg::parameters.threshold = is_valid_low_threshold ? Semg::parameters.threshold : SEMG_TRIGGER_THRESHOLD_MINIMUM;
}

bool Semg::impedanceTooLow() {
    return (Semg::output > SEMG_LOW_IMPEDANCE_THRESHOLD) ? true : false;
}

void Semg::samplingCallback(void * obj) {
	if (Semg::sample_amount < SEMG_SAMPLES_PER_VALUE) {
        Semg::readSensor();
		Semg::filtered_value[Semg::sample_amount] = Semg::voltage;
		Semg::sample_amount++;
	}
}

void Semg::filterSamplesArray() {
	for (int i = 0; i < SEMG_SAMPLES_PER_VALUE; i++) {
        Semg::filtered_value[i] = SemgFilter::filter(Semg::filtered_value[i]);
    }
}

void Semg::startSamplingTimer() {
	samplingTimer = xTimerCreate(
		"sEMG timer",           // Nome do temporizador (para fins de depuração)
		pdMS_TO_TICKS(Semg::sampling_period_ms),  // Período em milissegundos
		pdTRUE,              // Modo autoreload, o temporizador será recarregado automaticamente
		(void *)0,           // ID do temporizador (pode ser usado para identificação adicional)
		Semg::samplingCallback        // Função a ser chamada quando o temporizador expirar
    );

    // Verificação se o temporizador foi criado com sucesso
    if (samplingTimer != NULL) {
        xTimerStart(samplingTimer, 0);
        //Fes::fesLoop();
    } else {
        printDebug("Erro ao criar o temporizador do FES!");
    }
}

void Semg::stopSamplingTimer() {
	xTimerDelete(Semg::samplingTimer, 0);
}

float Semg::getFilteredSample() {

	Semg::startSamplingTimer();
	while (Semg::sample_amount < SEMG_SAMPLES_PER_VALUE) {}
	Semg::sample_amount = 0;

    Semg::filterSamplesArray();

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

    return (Semg::parameters.gain * sum_of_averages) / 10.0f;
}

float Semg::readSensor() {
    Semg::enableSensor();
    Semg::voltage = Adc::getValue(SEMG_ADC_PIN);
    Semg::disableSensor();
    return Semg::voltage;
}

float Semg::acquireAverage(int readings_amount) {
	Semg::output = 0;
    for (int i = 0; i < readings_amount; i++) {
        Semg::output += Semg::getFilteredSample();
    }
    Semg::output /= readings_amount;
    
    return Semg::output;
}

void Semg::testTrigger(int test_duration_seconds) {
	test_duration_seconds *= 1000;
	time_t test_start = millis();
	while (millis() - test_start < test_duration_seconds) {
		Semg::acquireAverage();
		if (Semg::isTrigger()) {
			Semg::sendTriggerMessage();
			return;
		}
	}
}

void Semg::sendTriggerMessage() {
	DynamicJsonDocument *message_document = new DynamicJsonDocument(JSON_OBJECT_SIZE(2));
    if (!message_document) {
        printDebug("teste tstes");
    }
    (*message_document)[MESSAGE_KEYS::CODE] = MESSAGE_CODE_TRIGGER;
    (*message_document)[MESSAGE_KEYS::METHOD] = MESSAGE_METHOD::WRITE;

    MessageHandler::sendMessage(message_document);
}

void Semg::enableSensor() {
    digitalWrite(SEMG_ENABLE_PIN, LOW);
}

void Semg::disableSensor() {
    digitalWrite(SEMG_ENABLE_PIN, HIGH);
}


