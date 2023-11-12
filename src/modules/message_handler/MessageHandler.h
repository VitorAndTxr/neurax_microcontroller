#ifndef MESSAGE_HANDLER
#define MESSAGE_HANDLER

#include <ArduinoJson.h>

#include "CommunicationProtocol.h"
#include "../debug/Debug.h"
#include "../gyroscope/Gyroscope.h"

static const uint8_t buf_len = 255;     // Size of buffer to look for command
static const char command[] = "delay "; // Note the space!


class MessageHandler
{
private:
    static const int queue_size;
    static QueueHandle_t message_handler_queue;
    static void interpretMessage(String data);
    static void handleHandshake();
    static void handleGyroscopeMessage(DynamicJsonDocument &message);
    static void handleSessionParametersMessage(DynamicJsonDocument &message);
    static int getMessageCode(DynamicJsonDocument &message);
    static String getMessageMethod(DynamicJsonDocument &message);
    static void handleIncomingMessages();
    static void handleOutgoingMessages();
public:
    MessageHandler() = delete;
    ~MessageHandler() = delete;
    static void init();
    static void loop();
    static bool addMessageToQueue(const DynamicJsonDocument* message);
    static bool readMessageFromQueue(DynamicJsonDocument* message);
};

#endif