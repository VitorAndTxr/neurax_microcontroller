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

void Session::sendSessionStatus(){
	DynamicJsonDocument *message_document = new DynamicJsonDocument(JSON_OBJECT_SIZE(14));

	
	(*message_document)[MESSAGE_KEYS::CODE] = SESSION_COMMANDS::STATUS;
	(*message_document)[MESSAGE_KEYS::METHOD] = MESSAGE_METHOD::WRITE;

	JsonObject status_message_body = 
		(*message_document).createNestedObject(MESSAGE_KEYS::BODY);

	JsonObject status_message_parameters = 
		status_message_body.createNestedObject(MESSAGE_KEYS::PARAMETERS);

	JsonObject status_message_status = 
		status_message_body.createNestedObject(MESSAGE_KEYS::STATUS);


	status_message_status[MESSAGE_KEYS::status::COMPLETE_STIMULI_AMOUNT] = 
		Session::status.complete_stimuli_amount;
	status_message_status[MESSAGE_KEYS::status::INTERRUPTED_STIMULI_AMOUNT] = 
		Session::status.interrupted_stimuli_amount;
	status_message_status[MESSAGE_KEYS::status::TIME_OF_LAST_TRIGGER] = 
		Session::status.time_of_last_trigger;
	status_message_status[MESSAGE_KEYS::status::SESSION_DURATION] = 
		Session::status.session_duration;

	status_message_parameters[MESSAGE_KEYS::parameters::AMPLITUDE] = 
		Fes::parameters.amplitude;
	status_message_parameters[MESSAGE_KEYS::parameters::FREQUENCY] =
		Fes::parameters.frequency;
	status_message_parameters[MESSAGE_KEYS::parameters::PULSE_WIDTH] =
		Fes::parameters.pulse_width_ms;
	status_message_parameters[MESSAGE_KEYS::parameters::DIFFICULTY] =
		Semg::parameters.difficulty;
	status_message_parameters[MESSAGE_KEYS::parameters::STIMULI_DURATION] =
		Fes::parameters.fes_duration_ms;

	MessageHandler::addMessageToQueue(message_document);
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