#ifndef SESSION_MODULE
#define SESSION_MODULE

#include <ArduinoJson.h>
#include "../semg/Semg.h"
#include "../fes/Fes.h"
#include "../message_handler/MessageHandler.h"
#include "../../globals.h"
#include "../emergency_button/EmergencyButton.h"
#include "../gyroscope/Gyroscope.h"
struct SessionStatus
{
    static short complete_stimuli_amount;
    static short interrupted_stimuli_amount;
    static volatile bool paused;
    static volatile bool ongoing;
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
    static SessionStatus status;
    static SessionParameters parameters;
	TickType_t getTicksDelayBetweenStimuli();
    static void resetSessionStatus(bool session_starting = true);
	static void delayBetweenStimuli();
	static void detectionAndStimulation();
	static TaskHandle_t task_handle;
	static void suspendSessionTask();
public:
    Session() = delete;
    ~Session() = delete;

	static void loop(void * parameters);
    static void init();
    static void start();
    static void stop();
    static void pause();
    static void resume();
    static void singleStimulus();
	static void sendSessionStatus();
};

#endif