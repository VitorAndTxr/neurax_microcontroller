#include "MessageHandler.h"


DynamicJsonDocument message(JSON_BUFFER_SIZE);
QueueHandle_t MessageHandler::message_handler_queue = NULL; 
TaskHandle_t MessageHandler::task_handle = NULL;


void MessageHandler::init() {
    Bluetooth::init();
    if(!Bluetooth::isConnected) {
        Bluetooth::waitForConnection();
    }
}

void MessageHandler::start() {
    // TODO check memory
    xTaskCreatePinnedToCore(
		MessageHandler::loop,
		"Session task",
		4096,
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
        }
        else {
            Bluetooth::waitForConnection();
        }
    }
}

void MessageHandler::sendMessage(DynamicJsonDocument* message) {
    String serialized_message;
    serializeJson(*message, serialized_message);
    printDebug("[MSG] Serializado: ");
    printDebug(serialized_message);
    if (Bluetooth::isConnected()) {
        printDebug("[MSG] Enviando...");
	    Bluetooth::sendData(serialized_message);
        printDebug("[MSG] Enviado!");
    }
    delay(200);
    //(*message).clear();
}

void MessageHandler::handleIncomingMessages() {
    String data = Bluetooth::readData();

    if (!data.isEmpty()) {
        printDebug("Dado recebido -->");
        printDebug(data);
        MessageHandler::interpretMessage(data);
    }
}

void MessageHandler::interpretMessage(String data)
{
    message.clear();
    DeserializationError error = deserializeJson(message, data);

    if (error) {
        printDebug("[MSG] interpretMessage: Error parsing JSON: ");
        printDebug(error.c_str());
        return;
    } 
    
    switch (getMessageCode(message)) {
        case GYROSCOPE_MESSAGE:
			printDebug("[MSG] Giroscópio");
            MessageHandler::handleGyroscopeMessage(message);
            break;
            
        case (SESSION_COMMANDS::START):
			printDebug("[MSG] SESSION_COMMANDS::START");

            Session::start();
            break;

        case SESSION_COMMANDS::STOP:
			printDebug("[MSG] SESSION_COMMANDS::STOP");
            Session::stop();
            break;

        case SESSION_COMMANDS::PAUSE:
			printDebug("[MSG] SESSION_COMMANDS::PAUSE");
		
            Session::pause();
            break;

        case SESSION_COMMANDS::RESUME:
			printDebug("[MSG] SESSION_COMMANDS::RESUME");
            Session::resume();
            break;

        case SESSION_COMMANDS::SINGLE_STIMULUS:
			printDebug("[msg] SESSION_COMMANDS::SINGLE_STIMULUS");
            Fes::begin();
            break;

        case SESSION_COMMANDS::PARAMETERS:
			printDebug("[msg] SESSION_COMMANDS::PARAMETERS");
            MessageHandler::handleSessionParametersMessage(message);
            break;
		
		case MESSAGE_CODE_TRIGGER:
			printDebug("[MSG] Trigger test");
			Semg::testTrigger();
			break;

        default:
			printDebug("[MSG] Unknown message code");
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
            JsonObject statusObj = message[MESSAGE_KEYS::BODY].as<JsonObject>();

            String serialized_message;
            serializeJson(statusObj, serialized_message);

            //Gyroscope::sendLastValue();
            //Semg::sendTriggerMessage();
            //Session::sendSessionStatus();

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
