#include "Semg.h"

float Semg::filtered_value[SEMG_SAMPLES_PER_VALUE] = {0};
float Semg::mes[5] = {0};
float SemgParameters::gain = SEMG_DEFAULT_GAIN;
float SemgParameters::difficulty = SEMG_DIFFICULTY_DEFAULT;
float SemgParameters::threshold = 0;
float Semg::mes_a[2] = {0};
float Semg::mes_b[2] = {0};
float Semg::voltage = 0.0f;
float Semg::output = 0.0f;

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
	DynamicJsonDocument *message_document = new DynamicJsonDocument(JSON_OBJECT_SIZE(3));

    (*message_document)[MESSAGE_KEYS::CODE] = MESSAGE_CODE_TRIGGER;
    (*message_document)[MESSAGE_KEYS::METHOD] = MESSAGE_METHOD::WRITE;

    MessageHandler::addMessageToQueue(message_document);
}

void Semg::enableSensor() {
    digitalWrite(SEMG_ENABLE_PIN, HIGH);
}

void Semg::disableSensor() {
    digitalWrite(SEMG_ENABLE_PIN, LOW);
}


