#ifndef MESSAGE_HANDLER
#define MESSAGE_HANDLER

#include <ArduinoJson.h>
#include "CommunicationProtocol.h"
#include "../debug/Debug.h"
#include "../gyroscope/Gyroscope.h"

class MessageHandler
{
private:
public:
    MessageHandler() = delete;
    ~MessageHandler() = delete;
    static void init();
    static void loop();
    static void handleIncomingMessages();
    static void handleOutgoingMessages();
    static void interpretMessage(String data);
    static void handleHandshake();
    static void handleGyroscopeMessage(DynamicJsonDocument &message);
    static void handleSessionParametersMessage(DynamicJsonDocument &message);
    static int getMessageCode(DynamicJsonDocument &message);
    static String getMessageMethod(DynamicJsonDocument &message);

};

#endif