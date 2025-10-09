#include "Semg.h"

float Semg::filtered_value[SEMG_SAMPLES_PER_VALUE] = {0};
volatile float Semg::raw_value[SEMG_SAMPLES_PER_VALUE] = {0};
float Semg::mes[5] = {0};
float SemgParameters::gain = SEMG_DEFAULT_GAIN;
float SemgParameters::difficulty = 50;
//float SemgParameters::difficulty = SEMG_DIFFICULTY_DEFAULT;

float SemgParameters::threshold = 30;
float Semg::mes_a[2] = {0};
float Semg::mes_b[2] = {0};
volatile float Semg::voltage = 0.0f;
float Semg::output = 0.0f;
volatile int Semg::sample_amount = 0;
TimerHandle_t Semg::samplingTimer = NULL;
TimerHandle_t Semg::ledTriggerTimer = NULL;
TaskHandle_t Semg::task_handle = NULL;
const float Semg::sampling_period_ms = SEMG_SAMPLING_PERIOD;

// Streaming variables
float Semg::streaming_buffer[STREAMING_BUFFER_SIZE] = {0};
volatile int Semg::buffer_write_index = 0;
volatile int Semg::buffer_read_index = 0;
volatile bool Semg::streaming_active = false;
StreamingConfig Semg::streaming_config = {DEFAULT_STREAMING_RATE, STREAMING_RAW, 5, 10};
TaskHandle_t Semg::streaming_task_handle = NULL;
unsigned long Semg::streaming_start_time = 0;

Led LED_TRIGGER(LED_PIN_TRIGGER);


void Semg::init() {
	ESP_LOGI(TAG_SEMG, "Setup...");

    pinMode(SEMG_ENABLE_PIN, OUTPUT);
    Semg::enableSensor();

    //Semg::disableSensor();
	LED_TRIGGER.set(false);
    Semg::createLedTriggerTimer();
}
void Semg::startLedTrigger(){
    //turn on led
    LED_TRIGGER.set(true);
    //start o timer
    xTimerStart(Semg::ledTriggerTimer, 0);
}

void Semg::setDifficulty(int difficulty) {
	Semg::parameters.difficulty = (float)difficulty;

	ESP_LOGI(TAG_SEMG, "Updated difficulty to %lf", Semg::parameters.difficulty);
	Semg::updateTriggerThreshold();
}

void Semg::increaseDifficulty(int increment) {
	ESP_LOGI(TAG_SEMG, "Increasing difficulty");

    Semg::parameters.difficulty += increment;
    Semg::parameters.difficulty = (Semg::parameters.difficulty > SEMG_DIFFICULTY_MAXIMUM) ? SEMG_DIFFICULTY_MAXIMUM : Semg::parameters.difficulty;

	ESP_LOGI(TAG_SEMG, "Updated difficulty to %lf", Semg::parameters.difficulty);
    Semg::updateTriggerThreshold();
}

void Semg::decreaseDifficulty(int decrement) {
	ESP_LOGI(TAG_SEMG, "Decreasing difficulty");
    Semg::parameters.difficulty -= decrement;
    Semg::parameters.difficulty = (Semg::parameters.difficulty < SEMG_DIFFICULTY_MINIMUM) ? SEMG_DIFFICULTY_MINIMUM : Semg::parameters.difficulty;

	ESP_LOGI(TAG_SEMG, "Updated difficulty to %lf", Semg::parameters.difficulty);
    Semg::updateTriggerThreshold();
}

inline bool Semg::outputIsInInterval(float lower_limit, float higher_limit){
	bool in_interval = Semg::output >= Semg::parameters.threshold && Semg::output <= 300.0f;
    
	ESP_LOGI(TAG_FES, "Verify Semg %f lower limit: %f, upper limit: %f\nIn interval? %d", Semg::output, lower_limit, 300.0f, static_cast<int>(in_interval));

    return in_interval;
}

bool Semg::isTrigger() {
	bool trigger = (outputIsInInterval(Semg::parameters.threshold, SEMG_TRIGGER_THRESHOLD_MAXIMUM)
        && !Fes::isOn());

	if (trigger) {
		ESP_LOGI(TAG_SEMG, "==== Trigger detected ====");
		//vTaskSuspend(MessageHandler::task_handle);
        Semg::sendTriggerMessage();
        Semg::startLedTrigger();
        //Gyroscope::sendLastValue();
        //vTaskResume(MessageHandler::task_handle);
        //LED_TRIGGER.set(true);
		//LED_TRIGGER.turnOnFor(2000);
	}
    return trigger;
}

void Semg::updateTriggerThreshold() {
    Semg::parameters.threshold = (15*(Semg::parameters.difficulty)); //* Semg::mes[0]; 
    bool is_valid_low_threshold = Semg::parameters.threshold > SEMG_TRIGGER_THRESHOLD_MINIMUM;
    Semg::parameters.threshold = is_valid_low_threshold ? Semg::parameters.threshold : SEMG_TRIGGER_THRESHOLD_MINIMUM;

	ESP_LOGI(TAG_SEMG, "Updated trigger threshold to %lf", Semg::parameters.threshold);
}

bool Semg::impedanceTooLow() {
    return (Semg::output > SEMG_LOW_IMPEDANCE_THRESHOLD) ? true : false;
}

void Semg::samplingCallback(TimerHandle_t xTimer) {
	// Mode 1: Session active (original behavior)
    if (Session::status.ongoing) {
        vTaskResume(Semg::task_handle);
    }

    // Mode 2: Streaming active (write to circular buffer)
    if (streaming_active) {
        // Read ADC value
        float value = Adc::getValue(SEMG_ADC_PIN);

        // Apply filter if configured
        float processed_value = applyStreamingFilter(value);

        // Write to circular buffer
        writeToBuffer(processed_value);
    }
}

void Semg::ledTriggerCallback(TimerHandle_t xTimer) {
	//vTaskSuspend(Session::task_handle);
    LED_TRIGGER.set(0);
}

void Semg::filterSamplesArray() {
	for (int i = 0; i < SEMG_SAMPLES_PER_VALUE; i++) {
        Semg::filtered_value[i] = SemgFilter::filter(Semg::filtered_value[i]);
    }
}
void Semg::createLedTriggerTimer(){
    if (ledTriggerTimer == NULL) {
        ledTriggerTimer = xTimerCreate(
            "sEMG timer",           // Nome do temporizador (para fins de depuração)
            pdMS_TO_TICKS(1000),  // Período em milissegundos
            pdFALSE,              // Modo autoreload, o temporizador será recarregado automaticamente
            (void *)0,           // ID do temporizador (pode ser usado para identificação adicional)
            Semg::ledTriggerCallback        // Função a ser chamada quando o temporizador expirar
        );
   }

    // Verificação se o temporizador foi criado com sucesso
    if (samplingTimer != NULL) {
        ESP_LOGE(TAG_SEMG, "creating led trigger timer ");
        
    //    if ( xTimerStart(samplingTimer, 0) != pdPASS) {
    //         ESP_LOGE(TAG_SEMG, "Restarting sEmg timer!");
    //    }
		//ESP_LOGI(TAG_SEMG, "Sampling timer started");
    } else {
		ESP_LOGE(TAG_SEMG, "Error creating led trigger timer!");
    }

}
void Semg::startSamplingTimer() {
	//ESP_LOGI(TAG_SEMG, "Starting sampling timer");
    if (samplingTimer == NULL) {
        samplingTimer = xTimerCreate(
            "sEMG timer",           // Nome do temporizador (para fins de depuração)
            pdMS_TO_TICKS(Semg::sampling_period_ms),  // Período em milissegundos
            pdTRUE,              // Modo autoreload, o temporizador será recarregado automaticamente
            (void *)0,           // ID do temporizador (pode ser usado para identificação adicional)
            Semg::samplingCallback        // Função a ser chamada quando o temporizador expirar
        );
   }

    // Verificação se o temporizador foi criado com sucesso
    if (samplingTimer != NULL) {
       if ( xTimerStart(samplingTimer, 0) != pdPASS) {
            ESP_LOGE(TAG_SEMG, "Restarting sEmg timer!");
       }
		//ESP_LOGI(TAG_SEMG, "Sampling timer started");
    } else {
		ESP_LOGE(TAG_SEMG, "Error creating sEmg timer!");
    }
    //ESP_LOGE(TAG_SEMG, "Aquiiiiiiiiiii!");
}

void Semg::sensorTask(void * obj) {
	while(true) {
		if (Semg::sample_amount < SEMG_SAMPLES_PER_VALUE) {
			Semg::voltage = Adc::getValue(SEMG_ADC_PIN);
			//Semg::readSensor();
			Semg::raw_value[Semg::sample_amount] = Semg::voltage;
			Semg::sample_amount++;
		}
		else{
			vTaskResume(Session::task_handle);
		}
		vTaskSuspend(NULL);
	}
}

void Semg::stopSamplingTimer() {
    if (samplingTimer != NULL) {
        xTimerStop(Semg::samplingTimer, 0);
        //samplingTimer = NULL;
	    //xTimerDelete(Semg::samplingTimer, 0);
    }
}

float Semg::getFilteredSample() {
	Semg::sample_amount = 0;

	Semg::startSamplingTimer();
	vTaskSuspend(NULL);
    Semg::stopSamplingTimer();
	
	for (int i = 0; i < SEMG_SAMPLES_PER_VALUE; i++) {
		filtered_value[i] = raw_value[i];
        //Serial.println(filtered_value[i]);
	}

	
	Semg::sample_amount = 0;

    Semg::filterSamplesArray();

    float average_every_n_samples;
    float sum_of_averages = 0;

    for (int i = 0; i < SEMG_SAMPLES_PER_VALUE/SEMG_SAMPLES_PER_AVERAGE; i++) {
        average_every_n_samples = 0;
        for (int j = 0; j < SEMG_SAMPLES_PER_AVERAGE; j++) {
            average_every_n_samples += Semg::filtered_value[SEMG_SAMPLES_PER_AVERAGE*i + j];
        }
        average_every_n_samples /= SEMG_SAMPLES_PER_AVERAGE;
        sum_of_averages += average_every_n_samples;
    }
    Serial.println(sum_of_averages);
    return (Semg::parameters.gain * sum_of_averages) / 10.0f;
}

float Semg::readSensor() {
    //Semg::enableSensor();
    Semg::voltage = Adc::getValue(SEMG_ADC_PIN);
    //Semg::disableSensor();
    return Semg::voltage;
}

float Semg::acquireAverage(int readings_amount) {
	Semg::output = 0;
	Semg::sample_amount = 0;
    
    for (int i = 0; i < readings_amount; i++) {
        Semg::output += Semg::getFilteredSample();
    }
    
    Semg::output /= (float)readings_amount;
    //Serial.print("output>");
    //Serial.println(Semg::output);
    
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
    digitalWrite(SEMG_ENABLE_PIN, HIGH);
}

void Semg::disableSensor() {
    digitalWrite(SEMG_ENABLE_PIN, LOW);
}

// ============================================================================
// STREAMING IMPLEMENTATION
// ============================================================================

void Semg::configureStreaming(int rate, const char* type_str) {
	ESP_LOGI(TAG_SEMG, "Configuring streaming: rate=%d Hz, type=%s", rate, type_str);

    streaming_config.rate = rate;

    // Parse type string
    if (strcmp(type_str, "raw") == 0) {
        streaming_config.type = STREAMING_RAW;
    } else if (strcmp(type_str, "filtered") == 0) {
        streaming_config.type = STREAMING_FILTERED;
    } else if (strcmp(type_str, "rms") == 0) {
        streaming_config.type = STREAMING_RMS;
    } else {
        streaming_config.type = STREAMING_RAW; // Default
		ESP_LOGW(TAG_SEMG, "Unknown streaming type '%s', defaulting to 'raw'", type_str);
    }

    // Calculate packets per second based on rate and samples per packet
    streaming_config.samples_per_packet = (rate >= 100) ? 10 : 5;
    streaming_config.packets_per_second = rate / streaming_config.samples_per_packet;

	ESP_LOGI(TAG_SEMG, "Streaming config: %d samples/packet, %d packets/second",
             streaming_config.samples_per_packet, streaming_config.packets_per_second);
}

void Semg::enableStreaming() {
	ESP_LOGI(TAG_SEMG, "Enabling streaming...");

    // Reset buffer indices
    buffer_write_index = 0;
    buffer_read_index = 0;
    streaming_active = true;
    streaming_start_time = millis();

    // Start sampling timer if not already running
    Semg::startSamplingTimer();

    // Create streaming task
    BaseType_t result = xTaskCreatePinnedToCore(
        Semg::streamingTask,
        "sEMG Streaming",
        4096,
        NULL,
        10, // Priority lower than MessageHandler
        &streaming_task_handle,
        1   // Core 1 (same as MessageHandler)
    );

    if (result == pdPASS) {
		ESP_LOGI(TAG_SEMG, "Streaming task created successfully");
    } else {
		ESP_LOGE(TAG_SEMG, "Failed to create streaming task");
        streaming_active = false;
    }
}

void Semg::disableStreaming() {
	ESP_LOGI(TAG_SEMG, "Disabling streaming...");

    streaming_active = false;

    // Delete streaming task if it exists
    if (streaming_task_handle != NULL) {
        vTaskDelete(streaming_task_handle);
        streaming_task_handle = NULL;
		ESP_LOGI(TAG_SEMG, "Streaming task deleted");
    }
}

bool Semg::isStreaming() {
    return streaming_active;
}

int Semg::getAvailableSamples() {
    // Calculate available samples in circular buffer
    int write = buffer_write_index;
    int read = buffer_read_index;

    if (write >= read) {
        return write - read;
    } else {
        return (STREAMING_BUFFER_SIZE - read) + write;
    }
}

void Semg::readStreamingSamples(float* output, int count) {
    for (int i = 0; i < count; i++) {
        output[i] = streaming_buffer[buffer_read_index];
        buffer_read_index = (buffer_read_index + 1) % STREAMING_BUFFER_SIZE;
    }
}

void Semg::writeToBuffer(float value) {
    streaming_buffer[buffer_write_index] = value;
    buffer_write_index = (buffer_write_index + 1) % STREAMING_BUFFER_SIZE;

    // Check for buffer overflow (write catching up to read)
    if (buffer_write_index == buffer_read_index) {
		ESP_LOGW(TAG_SEMG, "Streaming buffer overflow! Dropping oldest sample.");
        buffer_read_index = (buffer_read_index + 1) % STREAMING_BUFFER_SIZE;
    }
}

float Semg::applyStreamingFilter(float value) {
    switch (streaming_config.type) {
        case STREAMING_RAW:
            return value;

        case STREAMING_FILTERED:
            return SemgFilter::filter(value);

        case STREAMING_RMS:
            // Simple RMS: return absolute value (full RMS would require windowing)
            return fabs(value);

        default:
            return value;
    }
}

void Semg::sendStreamingMessage(float* samples, int count) {
    // Create JSON document
    DynamicJsonDocument *message_document = new DynamicJsonDocument(JSON_BUFFER_SIZE);

    (*message_document)[MESSAGE_KEYS::CODE] = SEMG_STREAMING::STREAM_DATA;
    (*message_document)[MESSAGE_KEYS::METHOD] = MESSAGE_METHOD::WRITE;

    JsonObject body = (*message_document).createNestedObject(MESSAGE_KEYS::BODY);
    body[MESSAGE_KEYS::streaming::TIMESTAMP] = millis();

    JsonArray values_array = body.createNestedArray(MESSAGE_KEYS::streaming::VALUES);
    for (int i = 0; i < count; i++) {
        // Round to 1 decimal place to save bandwidth
        values_array.add(roundf(samples[i] * 10.0f) / 10.0f);
    }

    // Send via MessageHandler
    MessageHandler::sendMessage(message_document);
}

void Semg::streamingTask(void* parameters) {
	ESP_LOGI(TAG_SEMG, "Streaming task started");

    TickType_t last_send = xTaskGetTickCount();
    float samples[MAX_SAMPLES_PER_PACKET];

    while (streaming_active) {
        // Check timeout
        unsigned long elapsed_minutes = (millis() - streaming_start_time) / 60000;
        if (elapsed_minutes >= STREAMING_TIMEOUT_MINUTES) {
			ESP_LOGW(TAG_SEMG, "Streaming timeout reached (%d minutes), stopping...", STREAMING_TIMEOUT_MINUTES);
            Semg::disableStreaming();
            break;
        }

        // Wait for next send interval
        TickType_t interval = pdMS_TO_TICKS(1000 / streaming_config.packets_per_second);
        vTaskDelayUntil(&last_send, interval);

        // Check if enough samples are available
        int available = getAvailableSamples();
        if (available >= streaming_config.samples_per_packet) {
            // Read samples from buffer
            readStreamingSamples(samples, streaming_config.samples_per_packet);

            // Send via Bluetooth
            if (Bluetooth::isConnected()) {
                sendStreamingMessage(samples, streaming_config.samples_per_packet);
            } else {
				ESP_LOGW(TAG_SEMG, "Bluetooth disconnected, stopping streaming");
                Semg::disableStreaming();
                break;
            }
        } else {
			ESP_LOGD(TAG_SEMG, "Not enough samples (%d/%d), skipping packet",
                     available, streaming_config.samples_per_packet);
        }
    }

	ESP_LOGI(TAG_SEMG, "Streaming task finished");
    vTaskDelete(NULL);
}


