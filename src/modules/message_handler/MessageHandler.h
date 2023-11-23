#ifndef MESSAGE_HANDLER
#define MESSAGE_HANDLER

#include <ArduinoJson.h>

#include "CommunicationProtocol.h"
#include "../debug/Debug.h"
#include "../gyroscope/Gyroscope.h"
#include "../bluetooth/Bluetooth.h"
#include "../session/Session.h"

static const char* TAG_MSG = "MSG";

class MessageHandler
{
private:
    static QueueHandle_t message_handler_queue;
    static void interpretMessage(String data);
    static void handleGyroscopeMessage(DynamicJsonDocument &message);
    static void handleSessionParametersMessage(DynamicJsonDocument &message);
    static int getMessageCode(DynamicJsonDocument &message);
    static String getMessageMethod(DynamicJsonDocument &message);
    static void handleIncomingMessages();
public:
    static TaskHandle_t task_handle;
    MessageHandler() = delete;
    ~MessageHandler() = delete;
    static void init();
    static void start();
    static void loop(void * parameters);
    static void sendMessage(DynamicJsonDocument* message);
};

#endif