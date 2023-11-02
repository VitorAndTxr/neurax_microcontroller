#ifndef SESSION_MODULE
#define SESSION_MODULE

#include "../semg/Semg.h"
#include "../fes/Fes.h"
struct SessionStatus
{
    static short completeStimuliAmount;
    static short interruptedStimuliAmount;
    static bool paused;
    static bool ongoing;
    static uint32_t time_of_last_trigger;
    static uint32_t session_duration;
};

struct SessionParameters
{
    short maximumStimuliAmount;
    short minimumSecondsBetweenStimuli;
    uint32_t maximum_duration;
    bool limited_by_duration;
    bool limited_by_amount_of_trigger;
};

class Session
{
private:
    static SessionStatus details;
    static SessionParameters parameters;
public:
    Session() = delete;
    ~Session() = delete;
    static void init();
    static void start();
    static void stop();
    
};

#endif