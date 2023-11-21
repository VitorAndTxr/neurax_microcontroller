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
        if (Bluetooth::isConnected()) {
            MessageHandler::handleIncomingMessages();
        }
        else {
            Bluetooth::waitForConnection();
        }
    }
}

void MessageHandler::sendMessage(DynamicJsonDocument* message) {
    String serialized_message;
    serializeJson(*message, serialized_message);

	ESP_LOGI(TAG_MSG, "Serialized message:");
	ESP_LOGI(TAG_MSG, "%s", serialized_message.c_str());

    if (Bluetooth::isConnected()) {
		ESP_LOGI(TAG_MSG, "Sending message...");
	    Bluetooth::sendData(serialized_message);
		ESP_LOGI(TAG_MSG, "Message sent!");
    }
    delay(200);
    //(*message).clear();
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
		
            Session::pause();
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
    }
}
