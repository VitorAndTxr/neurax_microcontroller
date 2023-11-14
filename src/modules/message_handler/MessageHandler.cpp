#include "MessageHandler.h"
#include "modules/bluetooth/Bluetooth.h"
#include "modules/session/Session.h"

DynamicJsonDocument message(JSON_BUFFER_SIZE);
const int MessageHandler::queue_size = 10;
QueueHandle_t MessageHandler::message_handler_queue = NULL; 
TaskHandle_t MessageHandler::task_handle = NULL;


void MessageHandler::init() {
    if(!Bluetooth::isConnected) {
        Bluetooth::waitForConnection();
    }
	// TODO trocar uint32 pra ponteiro pra msg
	message_handler_queue = xQueueCreate(queue_size, sizeof(uint32_t*));
	if (message_handler_queue == NULL) {
		printDebug("QueueHandler: Failed to create queue.");
	}
}

void MessageHandler::start() {
    // TODO check memory
    xTaskCreatePinnedToCore(
		MessageHandler::loop,
		"Session task",
		2048,
		NULL,
		MESSAGE_HANDLER_SESSION_TASK_PRIORITY,
		&MessageHandler::task_handle,
		secondary_cpu
	);

}

void MessageHandler::loop(void * parameters)
{
    while (true) {
        if (Bluetooth::isConnected()) {
            MessageHandler::handleIncomingMessages();
            MessageHandler::handleOutgoingMessages();
        }
        else {
            Bluetooth::waitForConnection();
        }
    }
}

bool MessageHandler::addMessageToQueue(const DynamicJsonDocument* message) {
	if (xQueueSend(message_handler_queue, message, portMAX_DELAY) != pdPASS) {
        printDebug("QueueHandler: Failed to message object to queue");
        return false;
    }
	return true;
}

bool MessageHandler::readMessageFromQueue(DynamicJsonDocument* message) {
    return (xQueueReceive(message_handler_queue, message, portMAX_DELAY) == pdPASS);
}

inline void MessageHandler::handleIncomingMessages() {
    String data = Bluetooth::readData();
    if (!data.isEmpty()) {
        MessageHandler::interpretMessage(data);
    }
}

inline void MessageHandler::handleOutgoingMessages() {
	DynamicJsonDocument* message_to_send = NULL;
    if (readMessageFromQueue(message_to_send)) {
        String serialized_message;
        serializeJson(*message_to_send, serialized_message);
        Bluetooth::sendData(serialized_message);
    }
    delete message_to_send;
}


void MessageHandler::interpretMessage(String data)
{
    DeserializationError error = deserializeJson(message, data);

    if (error) {
        printDebug("Error parsing JSON: ");
        printDebug(error.c_str());
        return;
    } 
    
    switch (getMessageCode(message)) {
        case GYROSCOPE_MESSAGE:
            MessageHandler::handleGyroscopeMessage(message);
            break;
            
        case (SESSION_COMMANDS::START):
            Session::start();
            break;

        case SESSION_COMMANDS::STOP:
            Session::stop();
            break;

        case SESSION_COMMANDS::PAUSE:
            Session::pause();
            break;

        case SESSION_COMMANDS::RESUME:
            Session::resume();
            break;

        case SESSION_COMMANDS::SINGLE_STIMULUS:
            Session::singleStimulus();
            break;

        case SESSION_COMMANDS::PARAMETERS:
            MessageHandler::handleSessionParametersMessage(message);
            break;
		
		case MESSAGE_CODE_TRIGGER:
			Semg::testTrigger();
			break;

        default:
            break;
    }
    message.clear();
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
            JsonObject statusObj = message[MESSAGE_KEYS::BODY][MESSAGE_KEYS::PARAMETERS].as<JsonObject>();


            float amplitude = statusObj[MESSAGE_KEYS::parameters::AMPLITUDE];
            float frequency = statusObj[MESSAGE_KEYS::parameters::FREQUENCY];
            float pulse_width = statusObj[MESSAGE_KEYS::parameters::PULSE_WIDTH];
            float difficulty = statusObj[MESSAGE_KEYS::parameters::DIFFICULTY];
            float fes_duration = statusObj[MESSAGE_KEYS::parameters::STIMULI_DURATION];

            Fes::parameters.amplitude = amplitude;
            Fes::parameters.frequency = frequency;
            Fes::parameters.pulse_width_ms = pulse_width;
            Fes::parameters.fes_duration_ms = fes_duration;

            Semg::parameters.difficulty = difficulty;
    }
}
