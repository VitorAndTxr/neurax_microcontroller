#ifndef MESSAGE_HANDLER
#define MESSAGE_HANDLER

#include <ArduinoJson.h>

class MessageHandler
{
private:
public:
    MessageHandler() = delete;
    ~MessageHandler() = delete;
    static void init();
};

#endif