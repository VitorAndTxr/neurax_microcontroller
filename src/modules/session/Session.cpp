#include "session.h"

short SessionStatus::completeStimuliAmount = 0;
short SessionStatus::interruptedStimuliAmount = 0;
uint32_t SessionStatus::time_of_last_trigger = 0;
volatile bool SessionStatus::paused = false;
volatile bool SessionStatus::ongoing = true;


void Session::init() {
	SessionStatus::paused = false;
	SessionStatus::ongoing = true;
    resetSessionStatus(false);
}

void Session::start() {
    resetSessionStatus();
	// start session thread (run function Session::loop on thread)
	// choose core carefully.
	/* xTaskCreateStaticPinnedToCore(

	) */
}

void Session::stop() {
	// kill session thread
}

void Session::pause() {
    Session::status.paused = true;
	// suspend session thread
	// send message to app (maybe add reason?)
}

void Session::resume() {
    Session::status.paused = false;
	// resume session thread
}

void Session::singleStimulus() {
}

TickType_t Session::getTicksDelayBetweenStimuli() {
	return Session::parameters.minimumSecondsBetweenStimuli * 1000 / portTICK_PERIOD_MS;
}

void Session::delayBetweenStimuli() {
	vTaskDelay(Session::parameters.minimumSecondsBetweenStimuli * 1000 / portTICK_PERIOD_MS);
}

void Session::loop() {
	while (Session::status.ongoing)
	{
		// read semg
		if (Semg::isTrigger()) {
			if (Semg::impedanceTooLow()){
				Session::pause();
			}
			Fes::begin();
			delayBetweenStimuli();
		}
	}
	

}

void Session::resetSessionStatus(bool session_starting) {
	SessionStatus::completeStimuliAmount = 0;
	SessionStatus::interruptedStimuliAmount = 0;
	SessionStatus::paused = !session_starting;
	SessionStatus::ongoing = session_starting;
	SessionStatus::time_of_last_trigger = 0;
}