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
StreamingConfig Semg::streaming_config = {DEFAULT_STREAMING_RATE, STREAMING_RAW, 10, 10};
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
        Serial.println(processed_value);

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
    
    SemgFilter::updateSamplingRate(2, 10, 40);

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

void Semg::startStreamingSamplingTimer(float period_ms) {
	//ESP_LOGI(TAG_SEMG, "Starting sampling timer");
    if (samplingTimer == NULL) {
        samplingTimer = xTimerCreate(
            "sEMG timer",           // Nome do temporizador (para fins de depuração)
            pdMS_TO_TICKS(period_ms),  // Período em milissegundos
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
        Semg::output += abs(Semg::getFilteredSample());
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

        // ✅ Configure filter ONCE when streaming is configured
        float sampling_time_ms = 1000.0f / (float)rate;
        SemgFilter::updateSamplingRate(sampling_time_ms, 10, 50, false);

        // ✅ Reset filter state to prevent oscillation
        SemgFilter::resetState();

        ESP_LOGI(TAG_SEMG, "Updated filter: %.2f ms period, 10-50 Hz bandpass + 60 Hz notch (sample rate: %.1f Hz)",
                 sampling_time_ms, 1000.0f / sampling_time_ms);

    } else if (strcmp(type_str, "rms") == 0) {
        streaming_config.type = STREAMING_RMS;
    } else {
        streaming_config.type = STREAMING_RAW; // Default
		ESP_LOGW(TAG_SEMG, "Unknown streaming type '%s', defaulting to 'raw'", type_str);
    }

    // Calculate packets per second based on rate and samples per packet
    streaming_config.samples_per_packet = 10 ;
    streaming_config.packets_per_second = rate / streaming_config.samples_per_packet;

	ESP_LOGI(TAG_SEMG, "Streaming config: %d samples/packet, %d packets/second",
             streaming_config.samples_per_packet, streaming_config.packets_per_second);
}

void Semg::enableStreaming() {
	ESP_LOGI(TAG_SEMG, "Enabling streaming...");
	ESP_LOGI(TAG_SEMG, "Current config: rate=%d Hz, type=%d, samples/pkt=%d, pkts/sec=%d",
             streaming_config.rate,
             streaming_config.type,
             streaming_config.samples_per_packet,
             streaming_config.packets_per_second);

    // Reset buffer indices
    buffer_write_index = 0;
    buffer_read_index = 0;
    streaming_active = true;
    streaming_start_time = millis();

	ESP_LOGI(TAG_SEMG, "Buffer reset: write_index=%d, read_index=%d",
             buffer_write_index, buffer_read_index);

    // Start sampling timer at configured streaming rate
    // For 20Hz streaming: period = 1000ms / 20Hz = 50ms
    float streaming_period_ms = 1000.0f / streaming_config.rate;
    Semg::startStreamingSamplingTimer(streaming_period_ms);

	ESP_LOGI(TAG_SEMG, "Sampling timer started (period=%.2f ms, rate=%d Hz)",
             streaming_period_ms, streaming_config.rate);

    // Check available heap before creating task
    ESP_LOGI(TAG_SEMG, "Free heap before task creation: %d bytes", esp_get_free_heap_size());

    // Create streaming task
    ESP_LOGI(TAG_SEMG, "Creating streaming task...");
    BaseType_t result = xTaskCreatePinnedToCore(
        Semg::streamingTask,
        "sEMG Streaming",
        4096,
        NULL,
        15, // Priority between MessageHandler (20) and other tasks
        &streaming_task_handle,
        1   // Core 1 (same as MessageHandler)
    );

	ESP_LOGI(TAG_SEMG, "xTaskCreatePinnedToCore returned: %d (pdPASS=%d)", result, pdPASS);

    if (result == pdPASS) {
		ESP_LOGI(TAG_SEMG, "Streaming task created successfully, handle=%p", streaming_task_handle);
    } else {
		ESP_LOGE(TAG_SEMG, "Failed to create streaming task, error code: %d", result);
        streaming_active = false;
    }

	ESP_LOGI(TAG_SEMG, "enableStreaming() finished");
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
    // Use critical section to avoid race conditions with ISR
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&mux);
    int write = buffer_write_index;
    int read = buffer_read_index;
    portEXIT_CRITICAL(&mux);

    if (write >= read) {
        return write - read;
    } else {
        return (STREAMING_BUFFER_SIZE - read) + write;
    }
}

void Semg::readStreamingSamples(float* output, int count) {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    for (int i = 0; i < count; i++) {
        portENTER_CRITICAL(&mux);
        output[i] = streaming_buffer[buffer_read_index];
        buffer_read_index = (buffer_read_index + 1) % STREAMING_BUFFER_SIZE;
        portEXIT_CRITICAL(&mux);
    }
}

void Semg::writeToBuffer(float value) {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL_ISR(&mux);

    streaming_buffer[buffer_write_index] = value;
    buffer_write_index = (buffer_write_index + 1) % STREAMING_BUFFER_SIZE;

    // Check for buffer overflow (write catching up to read)
    if (buffer_write_index == buffer_read_index) {
        // Buffer full - drop oldest sample
        buffer_read_index = (buffer_read_index + 1) % STREAMING_BUFFER_SIZE;

        // Log overflow (but limit frequency to avoid log spam)
        static unsigned long last_overflow_log = 0;
        if (millis() - last_overflow_log > 1000) {
            ESP_LOGW(TAG_SEMG, "Streaming buffer overflow! Dropping oldest samples.");
            last_overflow_log = millis();
        }
    }

    portEXIT_CRITICAL_ISR(&mux);
}

float Semg::applyStreamingFilter(float value) {
    switch (streaming_config.type) {
        case STREAMING_RAW:
            return value;

        case STREAMING_FILTERED: {
            // Use bandpass + notch filter to remove 60 Hz interference
            return SemgFilter::filter(value);
        }

        case STREAMING_RMS:
            return fabs(value);

        default:
            return value;
    }
}

bool Semg::sendStreamingMessage(float* samples, int count) {
    // Create JSON document
    DynamicJsonDocument *message_document = new DynamicJsonDocument(JSON_BUFFER_SIZE);

    (*message_document)[MESSAGE_KEYS::CODE] = SEMG_STREAMING::STREAM_DATA;
    // Removed "mt" field - redundant for streaming (always "w")
    // Saves 9 bytes per packet: 98→89 bytes

    JsonObject body = (*message_document).createNestedObject(MESSAGE_KEYS::BODY);
    body[MESSAGE_KEYS::streaming::TIMESTAMP] = millis();

    JsonArray values_array = body.createNestedArray(MESSAGE_KEYS::streaming::VALUES);
    for (int i = 0; i < count; i++) {
        // Round to 1 decimal place to save bandwidth
        float rounded_value = roundf(samples[i] * 10.0f) / 10.0f;
        values_array.add(rounded_value);

    }

    // Send via MessageHandler - return success/fail
    return MessageHandler::sendMessage(message_document);
}

void Semg::streamingTask(void* parameters) {
	ESP_LOGI(TAG_SEMG, "Streaming task started");
	ESP_LOGI(TAG_SEMG, "Config: %d samples/pkt, %d pkts/sec, interval: %d ms",
             streaming_config.samples_per_packet,
             streaming_config.packets_per_second,
             1000 / streaming_config.packets_per_second);

    float samples[MAX_SAMPLES_PER_PACKET];
    int packet_count = 0;
    int loop_count = 0;
    const int interval_ms = 1000 / streaming_config.packets_per_second;

    // ✅ Filter already configured in configureStreaming() - no need to update here

    while (streaming_active) {
        loop_count++;

        // Check timeout
        unsigned long elapsed_minutes = (millis() - streaming_start_time) / 60000;
        if (elapsed_minutes >= STREAMING_TIMEOUT_MINUTES) {
			ESP_LOGW(TAG_SEMG, "Streaming timeout reached (%d minutes), stopping...", STREAMING_TIMEOUT_MINUTES);
            Semg::disableStreaming();
            break;
        }

        // Check if enough samples are available
        int available = getAvailableSamples();

        if (available >= streaming_config.samples_per_packet) {
            // Read samples from buffer
            if (loop_count <= 10) {
                ESP_LOGI(TAG_SEMG, "Reading %d samples from buffer (available: %d)...",
                         streaming_config.samples_per_packet, available);
            }
            readStreamingSamples(samples, streaming_config.samples_per_packet);

            //Send via Bluetooth
            if (Bluetooth::isConnected()) {
                bool sent = sendStreamingMessage(samples, streaming_config.samples_per_packet);
                if (sent) {
                    packet_count++;
                } else {
					ESP_LOGW(TAG_SEMG, "Failed to send packet #%d, retrying next cycle", packet_count + 1);
                }
            } else {
				ESP_LOGW(TAG_SEMG, "Bluetooth disconnected, stopping streaming");
                Semg::disableStreaming();
                break;
            }
        } else {
            // Not enough samples yet - wait for next timer tick
            vTaskDelay(pdMS_TO_TICKS(interval_ms));
        }


    }

	ESP_LOGI(TAG_SEMG, "Streaming task finished (sent %d packets total)", packet_count);
    vTaskDelete(NULL);
}


