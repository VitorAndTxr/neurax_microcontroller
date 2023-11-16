#ifndef MESSAGE_HANDLER
#define MESSAGE_HANDLER

#include <ArduinoJson.h>

#include "CommunicationProtocol.h"
#include "../debug/Debug.h"
#include "../gyroscope/Gyroscope.h"
#include "../bluetooth/Bluetooth.h"
#include "../session/Session.h"

static const uint8_t buf_len = 255;     // Size of buffer to look for command
static const char command[] = "delay "; // Note the space!


class MessageHandler
{
private:
    static QueueHandle_t message_handler_queue;
    static void interpretMessage(String data);
    static void handleHandshake();
    static void handleGyroscopeMessage(DynamicJsonDocument &message);
    static void handleSessionParametersMessage(DynamicJsonDocument &message);
    static int getMessageCode(DynamicJsonDocument &message);
    static String getMessageMethod(DynamicJsonDocument &message);
    static TaskHandle_t task_handle;
    static void handleIncomingMessages();
public:
    MessageHandler() = delete;
    ~MessageHandler() = delete;
    static void init();
    static void start();
    static void loop(void * parameters);
    static void sendMessage(DynamicJsonDocument* message);
};

#endif