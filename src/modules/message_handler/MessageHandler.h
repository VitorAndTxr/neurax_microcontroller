#ifndef MESSAGE_HANDLER
#define MESSAGE_HANDLER

#include <ArduinoJson.h>
#include <vector>
#include "CommunicationProtocol.h"
#include "../debug/Debug.h"
#include "../gyroscope/Gyroscope.h"

static const uint8_t buf_len = 255;     // Size of buffer to look for command
static const char command[] = "delay "; // Note the space!
static const int message_handler_queue_size = 5;     // Size of msg_queue
static QueueHandle_t message_handler_queue;


class MessageHandler
{
private:
    static void interpretMessage(String data);
    static void handleHandshake();
    static void handleGyroscopeMessage(DynamicJsonDocument &message);
    static void handleSessionParametersMessage(DynamicJsonDocument &message);
    static int getMessageCode(DynamicJsonDocument &message);
    static String getMessageMethod(DynamicJsonDocument &message);
	static DynamicJsonDocument* getMessageParametersFragment(DynamicJsonDocument &message);
	static std::vector<DynamicJsonDocument* > message_queue;
public:
    MessageHandler() = delete;
    ~MessageHandler() = delete;
    static void init();
    static void loop();
    static void handleIncomingMessages();
    static void handleOutgoingMessages();

};

#endif