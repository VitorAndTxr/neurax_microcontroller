#include "Session.h"

short SessionStatus::complete_stimuli_amount = 0;
short SessionStatus::interrupted_stimuli_amount = 0;
uint32_t SessionStatus::time_of_last_trigger = 0;
uint32_t SessionStatus::session_duration = 0;
volatile bool SessionStatus::paused = false;
volatile bool SessionStatus::ongoing = false;
TaskHandle_t Session::task_handle = NULL;
SessionParameters Session::parameters;

void Session::init() {
	SessionStatus::paused = false;
	SessionStatus::ongoing = true;
    resetSessionStatus(false);

	Session::parameters.maximumStimuliAmount = 0xFF;
	Session::parameters.minimumSecondsBetweenStimuli = SESSION_DEFAULT_TIME_BETWEEN_STIMULI;
	Session::parameters.maximum_duration = 0xFFFFFFFF;
	Session::parameters.limited_by_amount_of_trigger = false;
	Session::parameters.limited_by_duration = false;
}

void Session::start() {
    resetSessionStatus();
 	Session::stop();
	EmergencyButton::init();

	xTaskCreatePinnedToCore(
		Session::loop,
		"Session task",
		512 
			+ sizeof(float) * SEMG_SAMPLES_PER_VALUE 
			+ sizeof(float) * SEMG_DEFAULT_READINGS_AMOUNT,
		NULL,
		SESSION_TASK_PRIORITY,
		&Session::task_handle,
		session_cpu
	);
}

void Session::stop() {
	if(Session::task_handle != NULL) {
		vTaskDelete(Session::task_handle);
		Session::task_handle = NULL; 
		EmergencyButton::stop();
		Fes::hBridgeReset();
	}
}

void Session::pause() {
    Session::status.paused = true;
	suspendSessionTask();
	if (Fes::isOn) {
		Fes::stopFes();
		Fes::hBridgeReset();
	}
	
	Session::status.interrupted_stimuli_amount++;
	Session::sendSessionStatus();
	// TODO
	// send pause message to app (maybe add reason?)
}

void Session::suspendSessionTask() {
	if (Session::task_handle != NULL) {
		vTaskSuspend(Session::task_handle);
	}
}

void Session::resume() {
    Session::status.paused = false;
	if (Session::task_handle != NULL) {
		vTaskResume(Session::task_handle);
	}
}

void Session::singleStimulus() {
	detectionAndStimulation();
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

void Session::loop(void * parameters) {
	while (Session::status.ongoing) {
		if (!Session::status.paused) {
			Session::detectionAndStimulation();
		}
	}
}

void Session::detectionAndStimulation() {
	Semg::acquireAverage();
	if (Semg::isTrigger()) {
		// TODO Leds
		if (Semg::impedanceTooLow()) {
			Session::pause();
			// TODO Leds in thread
		}
		else {
			Semg::sendTriggerMessage();
			Fes::begin();
			if(!Fes::emergency_stop) {
				Session::status.complete_stimuli_amount++;
				Session::sendSessionStatus();
			}

			Fes::emergency_stop = false;
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
	Fes::emergency_stop = false;
	Session::stop();

}