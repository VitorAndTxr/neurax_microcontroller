#include "session.h"
#include "Session.h"

short SessionStatus::complete_stimuli_amount = 0;
short SessionStatus::interrupted_stimuli_amount = 0;
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

void Session::sendSessionStatus( ){
	DynamicJsonDocument status_message(JSON_OBJECT_SIZE(11));

	StaticJsonDocument<200> message_document;
	
	message_document[MESSAGE_KEYS::CODE] = SESSION_COMMANDS::STATUS;
	message_document[MESSAGE_KEYS::METHOD] = MESSAGE_METHOD::WRITE;

	JsonObject status_message_body = 
		message_document.createNestedObject(MESSAGE_KEYS::BODY);

	JsonObject status_message_parameters = 
		status_message_body.createNestedObject(MESSAGE_KEYS::PARAMETERS);

	JsonObject status_message_status = 
		status_message_body.createNestedObject(MESSAGE_KEYS::STATUS);


	status_message_parameters[MESSAGE_KEYS::status::COMPLETE_STIMULI_AMOUNT] = 
		Session::status.complete_stimuli_amount;


	String serialized_message;
	serializeJson(message_document, serialized_message);

	
	//status
/* 	status_message[MESSAGE_KEYS::BODY]["status"]["csa"] = Session::status.complete_stimuli_amount;
	status_message[MESSAGE_KEYS::BODY]["status"]["isa"] = Session::status.interrupted_stimuli_amount;
	status_message[MESSAGE_KEYS::BODY]["status"]["tlt"] = Session::status.time_of_last_trigger;
	status_message[MESSAGE_KEYS::BODY]["status"]["sd"] = Session::status.session_duration; */
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
	SessionStatus::complete_stimuli_amount = 0;
	SessionStatus::interrupted_stimuli_amount = 0;
	SessionStatus::paused = !session_starting;
	SessionStatus::ongoing = session_starting;
	SessionStatus::time_of_last_trigger = 0;
}