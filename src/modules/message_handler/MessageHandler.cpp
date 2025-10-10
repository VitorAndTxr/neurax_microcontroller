#include "MessageHandler.h"


DynamicJsonDocument message(JSON_BUFFER_SIZE);
QueueHandle_t MessageHandler::message_handler_queue = NULL; 
TaskHandle_t MessageHandler::task_handle = NULL;


void MessageHandler::init() {
    Bluetooth::init();
    if(!Bluetooth::isConnected) {
		ESP_LOGI(TAG_MSG, "Bluetooth not connected.");
        Bluetooth::waitForConnection();
    }
}

void MessageHandler::start() {
    // TODO check memory
	BaseType_t xReturned;
	ESP_LOGI(TAG_MSG, "Creating Message Handler task...");

    xReturned = xTaskCreatePinnedToCore(
		MessageHandler::loop,
		"Session task",
		4096,
		NULL,
		MESSAGE_HANDLER_SESSION_TASK_PRIORITY,
		&MessageHandler::task_handle,
		secondary_cpu
	);

	if (xReturned == pdPASS) {
		ESP_LOGI(TAG_MSG, "Success creating Message Handler task.");
	}
	else {
		ESP_LOGE(TAG_MSG, "Error creating Message Handler task: errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY");
	}

}

void MessageHandler::loop(void * parameters)
{
	ESP_LOGI(TAG_MSG, "Starting Message Handler loop");


    while (true) {
        //ESP_LOGI(TAG_MSG, "no loop");

        if (Bluetooth::isConnected()) {
            MessageHandler::handleIncomingMessages();

        }
        else {
            Bluetooth::waitForConnection();
        }

        // Small delay to allow other tasks to run
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

bool MessageHandler::sendMessage(DynamicJsonDocument* message) {

    int message_code = (*message)[MESSAGE_KEYS::CODE];
    bool is_streaming_data = (message_code == SEMG_STREAMING::STREAM_DATA);

    // Check if this is an ACK message (method = "a")
    String method = (*message)[MESSAGE_KEYS::METHOD];
    bool is_ack = (method == "a");

    String serialized_message;
    serializeJson(*message, serialized_message);

    // Reduced logging for streaming data packets (too verbose)
    if (!is_streaming_data) {
		ESP_LOGI(TAG_MSG, "Serialized message:");
		ESP_LOGI(TAG_MSG, "%s", serialized_message.c_str());
    }

    bool success = false;
    if (Bluetooth::isConnected()) {
        if (!is_streaming_data) {
			ESP_LOGI(TAG_MSG, "Sending message...");
        }
	    success = Bluetooth::sendData(serialized_message);
        if (!is_streaming_data) {
            if (success) {
				ESP_LOGI(TAG_MSG, "Message sent!");
            } else {
				ESP_LOGW(TAG_MSG, "Message send failed!");
            }
        }
    }

    // Only delay for non-streaming messages AND non-ACK messages
    // ACK messages should not delay to avoid blocking streaming
    if (!is_streaming_data && !is_ack) {
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    if (message != NULL){
        delete message;
    }

    if (!is_streaming_data) {
		ESP_LOGI(TAG_MSG, "getting out message send");
    }

    return success;
}

void MessageHandler::handleIncomingMessages() {
    String data = Bluetooth::readData();

    if (!data.isEmpty()) {
		ESP_LOGI(TAG_MSG, "=== Received data ===");
		ESP_LOGI(TAG_MSG, "--->");
		ESP_LOGI(TAG_MSG, "%s", data.c_str());
        MessageHandler::interpretMessage(data);
    }
}

void MessageHandler::interpretMessage(String data)
{
    message.clear();
    DeserializationError error = deserializeJson(message, data);

    if (error) {
		ESP_LOGE(TAG_MSG, "Error parsing JSON: %s", error.c_str());
        return;
    } 
    
    switch (getMessageCode(message)) {
        case GYROSCOPE_MESSAGE:
			ESP_LOGI(TAG_MSG, "Giroscópio");
            MessageHandler::handleGyroscopeMessage(message);
            break;
            
        case (SESSION_COMMANDS::START):
			ESP_LOGI(TAG_MSG, "SESSION_COMMANDS::START");

            Session::start();
            break;

        case SESSION_COMMANDS::STOP:
			ESP_LOGI(TAG_MSG, "SESSION_COMMANDS::STOP");
            Session::stop();
            break;

        case SESSION_COMMANDS::PAUSE:
			ESP_LOGI(TAG_MSG, "SESSION_COMMANDS::PAUSE");
		
            Session::pauseFromMessageHandler();
            break;

        case SESSION_COMMANDS::RESUME:
			ESP_LOGI(TAG_MSG, "SESSION_COMMANDS::RESUME");
            Session::resume();
            break;

        case SESSION_COMMANDS::SINGLE_STIMULUS:
			ESP_LOGI(TAG_MSG, "SESSION_COMMANDS::SINGLE_STIMULUS");
            Fes::begin();
            break;

        case SESSION_COMMANDS::PARAMETERS:
			ESP_LOGI(TAG_MSG, "SESSION_COMMANDS::PARAMETERS");
            MessageHandler::handleSessionParametersMessage(message);
            break;
		
		case MESSAGE_CODE_TRIGGER:
			ESP_LOGI(TAG_MSG, "Trigger test");
			Semg::testTrigger();
			break;

        case SEMG_STREAMING::CONFIG_STREAM:
			ESP_LOGI(TAG_MSG, "SEMG_STREAMING::CONFIG_STREAM");
            MessageHandler::handleStreamingConfigMessage(message);
            MessageHandler::sendAck(SEMG_STREAMING::CONFIG_STREAM);
            break;

        case SEMG_STREAMING::START_STREAM:
			ESP_LOGI(TAG_MSG, "SEMG_STREAMING::START_STREAM");
            Semg::enableStreaming();
            MessageHandler::sendAck(SEMG_STREAMING::START_STREAM);
            break;

        case SEMG_STREAMING::STOP_STREAM:
			ESP_LOGI(TAG_MSG, "SEMG_STREAMING::STOP_STREAM");
            Semg::disableStreaming();
            MessageHandler::sendAck(SEMG_STREAMING::STOP_STREAM);
            break;

        default:
			ESP_LOGW(TAG_MSG, "Unknown message code");
            break;
    }
    //message.clear();
}

String MessageHandler::getMessageMethod(DynamicJsonDocument &message) {
    return message[MESSAGE_KEYS::METHOD];
}

int MessageHandler::getMessageCode(DynamicJsonDocument &message) {
    return message[MESSAGE_KEYS::CODE];
}

void MessageHandler::handleGyroscopeMessage(DynamicJsonDocument &message)
{
    switch (getMessageMethod(message)[0]) {
        case (MESSAGE_METHOD::READ):
            Gyroscope::sendLastValue();
            break;

        case (MESSAGE_METHOD::EXECUTE):
            Gyroscope::gyroscopeRoutine();
            Gyroscope::sendLastValue();
            break;

        case (MESSAGE_METHOD::ACK):
            // acknowledgement gyroscope data received from app
            break;
        default:
            break;
    }
}


void MessageHandler::handleSessionParametersMessage(DynamicJsonDocument &message) {
    if (getMessageMethod(message)[0] == MESSAGE_METHOD::WRITE) {
		ESP_LOGI(TAG_MSG, "Parsing received session parameters");

		JsonObject statusObj = message[MESSAGE_KEYS::BODY].as<JsonObject>();

		//String serialized_message;
		//serializeJson(statusObj, serialized_message);

		float amplitude = statusObj[MESSAGE_KEYS::parameters::AMPLITUDE];
		float frequency = statusObj[MESSAGE_KEYS::parameters::FREQUENCY];
		float pulse_width = statusObj[MESSAGE_KEYS::parameters::PULSE_WIDTH];
		float difficulty = statusObj[MESSAGE_KEYS::parameters::DIFFICULTY];
		float fes_duration = statusObj[MESSAGE_KEYS::parameters::STIMULI_DURATION];

		Fes::parameters.amplitude = amplitude;
		Fes::parameters.frequency = frequency;
		Fes::parameters.pulse_width_us = pulse_width;
		Fes::parameters.fes_duration_s = fes_duration;

		Semg::setDifficulty(difficulty);

		ESP_LOGD(TAG_MSG, "Amplitude: %lf", amplitude);
		ESP_LOGD(TAG_MSG, "Frequency: %lf", frequency);
		ESP_LOGD(TAG_MSG, "Pulse width (ms): %lf", pulse_width);
		ESP_LOGD(TAG_MSG, "FES stimulation duration (ms): %lf", fes_duration);
		ESP_LOGD(TAG_MSG, "SEMG difficulty: %lf", difficulty);

        Potentiometer::voltageSet(amplitude);
    }
}

void MessageHandler::handleStreamingConfigMessage(DynamicJsonDocument &message) {
    if (getMessageMethod(message)[0] == MESSAGE_METHOD::WRITE) {
		ESP_LOGI(TAG_MSG, "Parsing received streaming configuration");

		JsonObject body = message[MESSAGE_KEYS::BODY].as<JsonObject>();

		int rate = body[MESSAGE_KEYS::streaming::RATE] | DEFAULT_STREAMING_RATE;
		const char* type = body[MESSAGE_KEYS::streaming::TYPE] | "raw";

		ESP_LOGI(TAG_MSG, "Streaming config - Rate: %d Hz, Type: %s", rate, type);

		Semg::configureStreaming(rate, type);
    }
}

void MessageHandler::sendAck(int message_code) {
	ESP_LOGI(TAG_MSG, "Sending ACK for message code %d", message_code);

	DynamicJsonDocument *ack_message = new DynamicJsonDocument(JSON_BUFFER_SIZE);

	(*ack_message)[MESSAGE_KEYS::CODE] = message_code;
	(*ack_message)[MESSAGE_KEYS::METHOD] = "a";  // String literal to avoid ASCII serialization

	MessageHandler::sendMessage(ack_message);
}
