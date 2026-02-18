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

// Streaming variables - BINARY PROTOCOL (Fixed 215 Hz)
int16_t Semg::streaming_buffer[STREAMING_BUFFER_SIZE] = {0};
volatile int Semg::buffer_write_index = 0;
volatile int Semg::buffer_read_index = 0;
volatile bool Semg::streaming_active = false;
TaskHandle_t Semg::streaming_task_handle = NULL;
unsigned long Semg::streaming_start_time = 0;

Led LED_TRIGGER(LED_PIN_TRIGGER);


void Semg::init() {
	ESP_LOGI(TAG_SEMG, "Setup...");

    pinMode(SEMG_ENABLE_PIN, OUTPUT);
    Semg::enableSensor();

	LED_TRIGGER.set(false);
    Semg::createLedTriggerTimer();
}
void Semg::startLedTrigger(){
    LED_TRIGGER.set(true);
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
        Semg::sendTriggerMessage();
        Semg::startLedTrigger();
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
	// Only used for Session mode (FES trigger detection)
    // Streaming mode uses ADC continuous mode instead
    if (Session::status.ongoing) {
        vTaskResume(Semg::task_handle);
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

/**
 * @brief Convert float voltage to int16_t for binary protocol
 *
 * Maps ADC range (0-4.096V) to ±4096 integer range
 * Preserves millivolt precision: 1 LSB = 1 mV
 *
 * @param value ADC voltage in volts
 * @return int16_t value clamped to ±4096 range
 */
int16_t Semg::floatToInt16(float value) {
    // Clamp to valid range
    if (value > VALUE_RANGE_MAX) value = VALUE_RANGE_MAX;
    if (value < VALUE_RANGE_MIN) value = VALUE_RANGE_MIN;

    return (int16_t)value;
}


void Semg::enableStreaming() {
    // Idempotency guard: clean up any stale streaming state
    if (streaming_active || streaming_task_handle != NULL) {
        ESP_LOGW(TAG_SEMG, "Previous streaming session not cleaned up, stopping first");
        disableStreaming();
    }

    // Reset buffer
    buffer_write_index = 0;
    buffer_read_index = 0;
    streaming_active = true;
    streaming_start_time = millis();

    // Configure Butterworth filter for 215 Hz sampling
    float sampling_time_ms = 1000.0f / SEMG_FIXED_RATE_HZ;
    SemgFilter::updateSamplingRate(sampling_time_ms, 10, 50, false);
    SemgFilter::resetState();

    // Start ADC continuous mode (860 Hz → 215 Hz with 4x downsample)
    Adc::startContinuousMode(SEMG_ADC_PIN);

    // Create streaming task on Core 1
    BaseType_t result = xTaskCreatePinnedToCore(
        Semg::streamingTask,
        "sEMG Streaming",
        4096,
        NULL,
        15,  // Priority (higher than most, lower than MessageHandler)
        &streaming_task_handle,
        1    // Core 1
    );

    if (result != pdPASS) {
		ESP_LOGE(TAG_SEMG, "Failed to create streaming task (error: %d)", result);
        streaming_active = false;
        Adc::stopContinuousMode();
    }
}

void Semg::disableStreaming() {
    streaming_active = false;

    // Wait for streaming task to self-exit (up to 100ms)
    int timeout = 100;
    while (streaming_task_handle != NULL && timeout-- > 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // Force-delete only if task didn't self-exit
    if (streaming_task_handle != NULL) {
        ESP_LOGW(TAG_SEMG, "Streaming task did not self-exit, force deleting");
        vTaskDelete(streaming_task_handle);
        streaming_task_handle = NULL;
        // ADC must also be stopped since post-loop cleanup didn't run
        Adc::stopContinuousMode();
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

void Semg::readStreamingSamples(int16_t* output, int count) {
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    for (int i = 0; i < count; i++) {
        portENTER_CRITICAL(&mux);
        output[i] = streaming_buffer[buffer_read_index];
        buffer_read_index = (buffer_read_index + 1) % STREAMING_BUFFER_SIZE;
        portEXIT_CRITICAL(&mux);
    }
}

void Semg::writeToBuffer(int16_t value) {
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


/**
 * @brief Send streaming data via BINARY protocol (NEW - Option B)
 *
 * Packet Structure:
 *   Header (8 bytes): magic | code | timestamp | sample_count
 *   Data (100 bytes): int16_t[50]
 *   Total: 108 bytes (vs 282 bytes JSON = 72% reduction)
 *
 * @param samples Array of int16_t values
 * @param count Number of samples
 * @return true if sent successfully
 */
bool Semg::sendBinaryStreamingMessage(int16_t* samples, int count) {
    // Calculate packet size
    const int packet_size = sizeof(BinaryPacketHeader) + (count * sizeof(int16_t));

    // Allocate buffer on stack (108 bytes max)
    uint8_t buffer[MAX_BINARY_PACKET_SIZE];

    // Build header
    BinaryPacketHeader* header = (BinaryPacketHeader*)buffer;
    header->magic = PACKET_MAGIC_BYTE;
    header->message_code = PACKET_MESSAGE_CODE_STREAM_DATA;
    header->timestamp = millis();
    header->sample_count = count;

    // Copy data payload
    memcpy(buffer + sizeof(BinaryPacketHeader), samples, count * sizeof(int16_t));

    // Send raw binary data via Bluetooth
    return Bluetooth::sendRawData(buffer, packet_size);
}

void Semg::streamingTask(void* parameters) {
    int16_t samples[SEMG_SAMPLES_PER_PACKET];
    int packet_count = 0;
    unsigned long samples_processed = 0;

    while (streaming_active) {
        // Check timeout (10 minutes)
        unsigned long elapsed_minutes = (millis() - streaming_start_time) / 60000;
        if (elapsed_minutes >= STREAMING_TIMEOUT_MINUTES) {
            ESP_LOGW(TAG_SEMG, "Streaming timeout reached (%d minutes), stopping...", STREAMING_TIMEOUT_MINUTES);
            streaming_active = false;
            break;
        }

        // Poll ADC for new averaged sample (215 Hz output from 860 Hz ADC)
        if (Adc::hasNewSample()) {
            // Get averaged sample (already downsampled 4x by ADC task)
            int16_t raw_sample = Adc::getLastSample();

            // Apply Butterworth filter (10-50 Hz bandpass) + Notch 60 Hz
            float filtered = SemgFilter::filterWithNotch((float)raw_sample);

            // Print filtered value to serial for verification (215 Hz)
            Serial.println(filtered);

            // Convert to int16_t and write to circular buffer
            int16_t final_sample = floatToInt16(filtered);
            writeToBuffer(final_sample);
            samples_processed++;
        }

        // Check if enough samples are available for packet transmission
        int available = getAvailableSamples();
        if (available >= SEMG_SAMPLES_PER_PACKET) {
            // Read samples from buffer
            readStreamingSamples(samples, SEMG_SAMPLES_PER_PACKET);

            // Send via Bluetooth (binary protocol)
            if (Bluetooth::isConnected()) {
                bool sent = sendBinaryStreamingMessage(samples, SEMG_SAMPLES_PER_PACKET);
                if (sent) {
                    packet_count++;
                } else {
                    ESP_LOGW(TAG_SEMG, "Failed to send packet #%d, retrying next cycle", packet_count + 1);
                }
            } else {
                ESP_LOGW(TAG_SEMG, "Bluetooth disconnected, stopping streaming");
                streaming_active = false;
                break;
            }
        }

        // Yield CPU (adaptive delay based on buffer fullness)
        vTaskDelay(pdMS_TO_TICKS(available < 10 ? 10 : 1));
    }

    // Post-loop cleanup: stop ADC and self-delete
    Adc::stopContinuousMode();
    streaming_task_handle = NULL;
    vTaskDelete(NULL);
}


