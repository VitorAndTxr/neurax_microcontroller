#include "MessageHandler.h"
#include "modules/bluetooth/Bluetooth.h"
#include "modules/session/Session.h"

DynamicJsonDocument message(JSON_BUFFER_SIZE);

void MessageHandler::init() {
    if(!Bluetooth::isConnected) {
        Bluetooth::waitForConnection();
    }
}

void MessageHandler::loop()
{
    while (true) {
        if (Bluetooth::isConnected()) {
            MessageHandler::handleIncomingMessages();
            MessageHandler::handleOutgoingMessages();
			// check connection! How?
        }
        else {
            Bluetooth::waitForConnection();
        }
    }
}

inline void MessageHandler::handleIncomingMessages() {
    String data = Bluetooth::readData();
    if (!data.isEmpty()) {
        MessageHandler::interpretMessage(data);
    }
}

inline void MessageHandler::handleOutgoingMessages() {
	// See if there's a message in the queue (do not block)
	DynamicJsonDocument* message_to_send;
    if (xQueueReceive(message_handler_queue, (void *)&message_to_send, 0) == pdTRUE) {
      String serialized_message;
	  serializeJson(*message_to_send, serialized_message);
	  // delete message to send
	  Bluetooth::sendData(serialized_message);
    }
}


void MessageHandler::interpretMessage(String data)
{
    DeserializationError error = deserializeJson(message, data);

    if (error) {
        printDebug("Error parsing JSON: ");
        printDebug(error.c_str());
        return;
    } 
    
    // aqui não vai ter case 'w'
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
        
        case NE_ACK:
            break;

        default:
            break;
    }
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
            Gyroscope::acquire();
            Gyroscope::sendLastValue();
            break;

        case (MESSAGE_METHOD::ACK):
            // acknowledgement gyroscope data received from app
            break;
        default:
            break;
    }
}


DynamicJsonDocument* MessageHandler::getMessageParametersFragment(DynamicJsonDocument &message) {
	DynamicJsonDocument* parametersFragment = new DynamicJsonDocument(JSON_OBJECT_SIZE(5));
    *parametersFragment = message[MESSAGE_KEYS::BODY][MESSAGE_KEYS::PARAMETERS];
    return parametersFragment;
}

void MessageHandler::handleSessionParametersMessage(DynamicJsonDocument &message) {
    if (getMessageMethod(message)[0] == MESSAGE_METHOD::WRITE) {
			DynamicJsonDocument* message_parameters = getMessageParametersFragment(message);

            float amplitude = (*message_parameters)[MESSAGE_KEYS::parameters::AMPLITUDE];
            float frequency = (*message_parameters)[MESSAGE_KEYS::parameters::FREQUENCY];
            float pulse_width = (*message_parameters)[MESSAGE_KEYS::parameters::PULSE_WIDTH];
            float difficulty = (*message_parameters)[MESSAGE_KEYS::parameters::DIFFICULTY];
            float stimuli_duration = (*message_parameters)[MESSAGE_KEYS::parameters::STIMULI_DURATION];

			// TODO
			// set the fes and session parameters
    }
}
