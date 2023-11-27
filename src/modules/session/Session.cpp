#include "Session.h"


short SessionStatus::complete_stimuli_amount = 0;
short SessionStatus::interrupted_stimuli_amount = 0;
uint32_t SessionStatus::time_of_last_trigger = 0;
uint32_t SessionStatus::session_duration = 0;
volatile bool SessionStatus::paused = false;
volatile bool SessionStatus::ongoing = false;
TaskHandle_t Session::task_handle = NULL;
SessionParameters Session::parameters;

static const char* TAG_SESSION = "SESSION";

void Session::init() {
	ESP_LOGI(TAG_SESSION, "Setup...");

	SessionStatus::paused = false;
	SessionStatus::ongoing = true;
    resetSessionStatus(false);

	Session::parameters.maximumStimuliAmount = 0xFF;
	Session::parameters.minimumSecondsBetweenStimuli = SESSION_DEFAULT_TIME_BETWEEN_STIMULI;
	Session::parameters.maximum_duration = 0xFFFFFFFF;
	Session::parameters.limited_by_amount_of_trigger = false;
	Session::parameters.limited_by_duration = false;
	ESP_LOGI(TAG_SESSION, "Setup done");
}

void createSessionTask() {
	ESP_LOGI(TAG_SESSION, "Creating session task...");
	if(xTaskCreatePinnedToCore(
		Session::loop,
		"Session task",
		4048//512 
			+ sizeof(float) * SEMG_SAMPLES_PER_VALUE 
			+ sizeof(float) * SEMG_DEFAULT_READINGS_AMOUNT,
		NULL,
		20,//SESSION_TASK_PRIORITY,
		&Session::task_handle,
		0
	)!= pdPASS){
		ESP_LOGE(TAG_SESSION, "Fail to create session thread");
	}
}

void createSensorTask() {
	ESP_LOGI(TAG_SESSION, "Creating Semg task...");
	if(xTaskCreatePinnedToCore(
		Semg::sensorTask,
		"Sensor task",
		4048//512 
			+ sizeof(float) * SEMG_SAMPLES_PER_VALUE 
			+ sizeof(float) * SEMG_DEFAULT_READINGS_AMOUNT,
		NULL,
		10,//SENSOR_TASK_PRIORITY,
		&Semg::task_handle,
		0
	)!= pdPASS){
		ESP_LOGE(TAG_SESSION, "Fail to create sampling thread");
	}
	vTaskSuspend(Semg::task_handle);
}

void Session::start() {
    resetSessionStatus();
	EmergencyButton::init();
	createSensorTask();
	createSessionTask();
	//Session::pauseFromMessageHandler();
	ESP_LOGI(TAG_SESSION, "Session Start");
}

void Session::stop() {
	if(Session::task_handle != NULL) {
		vTaskDelete(Session::task_handle);
		Session::task_handle = NULL; 
		EmergencyButton::stop();
		Fes::hBridgeReset();
	}
	ESP_LOGI(TAG_SESSION, "Stopped session.");
}

void Session::pauseFromSession() {
	ESP_LOGI(TAG_SESSION, "pausando");
    Session::status.paused = true;
	if (Fes::isOn) {
		Fes::stopFes();
		Session::status.interrupted_stimuli_amount++;
		delay(1);	
	}
	Fes::hBridgeReset();
	vTaskSuspend(NULL);
	//Session::sendSessionStatus();
	// TODO
	// send pause message to app (maybe add reason?)
	//Session::sendSessionPauseMessage();
}

void Session::pauseFromMessageHandler() {
	//ESP_LOGI(TAG_SESSION, "p");
    //Session::suspendSessionTask();
	Fes::stimulating = false;
	Fes::emergency_stop = true;
	//Session::status.paused = true;
	if (Fes::isOn) {
		//Fes::stopFes();
		Session::status.interrupted_stimuli_amount++;
		//delay(1);	
	}
	Fes::hBridgeReset();
	
	//Session::sendSessionStatus();
	// TODO
	// send pause message to app (maybe add reason?)
	//Session::sendSessionPauseMessage();
}

void Session::suspendSessionTask() {
	if (Session::task_handle != NULL) {
		//ESP_LOGD(TAG_SESSION, "Session task suspended.");
		vTaskSuspend(Session::task_handle);	
	}
}

void Session::resume() {
    Session::status.paused = false;
	Fes::emergency_stop = false;
	if (Session::task_handle != NULL) {
		Semg::sample_amount = 0;

		vTaskResume(Session::task_handle);		
		ESP_LOGD(TAG_SESSION, "Session task resumed.");

	}
	else{
		ESP_LOGD(TAG_SESSION, "Session resume was called but there was no task \" ==null\" ");
	}
}

void Session::singleStimulus() {
	Fes::begin();
}

void Session::sendSessionPauseMessage() {
	ESP_LOGI(TAG_SESSION, "Building pause message...");
    DynamicJsonDocument *message_document = new DynamicJsonDocument(JSON_OBJECT_SIZE(3));

    (*message_document)[MESSAGE_KEYS::CODE] = SESSION_COMMANDS::PAUSE;;
    (*message_document)[MESSAGE_KEYS::METHOD] = MESSAGE_METHOD::WRITE;

	ESP_LOGI(TAG_SESSION, "Sending session paused message...");
    MessageHandler::sendMessage(message_document);
}

void Session::sendSessionStatus(){

	ESP_LOGD(TAG_SESSION, "Building session status message...");
	
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
		Potentiometer::getCorrectedVoltage();
	status_message_parameters[MESSAGE_KEYS::parameters::FREQUENCY] =
		Fes::parameters.frequency;
	status_message_parameters[MESSAGE_KEYS::parameters::PULSE_WIDTH] =
		Fes::parameters.pulse_width_us;
	status_message_parameters[MESSAGE_KEYS::parameters::DIFFICULTY] =
		Semg::parameters.difficulty;
	status_message_parameters[MESSAGE_KEYS::parameters::STIMULI_DURATION] =
		Fes::parameters.fes_duration_s;

	ESP_LOGD(TAG_SESSION, "Sending session status message...");
	MessageHandler::sendMessage(message_document);
}

TickType_t Session::getTicksDelayBetweenStimuli() {
    return Session::parameters.minimumSecondsBetweenStimuli * 1000 / portTICK_PERIOD_MS;
}

void Session::delayBetweenStimuli() {
	vTaskDelay(Session::parameters.minimumSecondsBetweenStimuli * 1000 / portTICK_PERIOD_MS);
}

void Session::loop(void * parameters) {
	ESP_LOGD(TAG_SESSION, "Session task started.");

	while (Session::status.ongoing) {
		if (!Session::status.paused) {
			Session::detectionAndStimulation();
		}
	}
	Serial.println("fechando a thread");
	vTaskDelete(NULL);
}


void Session::detectionAndStimulation() {
	
	Semg::acquireAverage(5);
	bool i = false;
	bool isTrigger = Semg::isTrigger();
	ESP_LOGI(TAG_SEMG, "Variavel istrigger = %d ", isTrigger);
	if (isTrigger) {
		ESP_LOGI(TAG_SEMG, "==== teste ====");
		if (!i){
			i=true;
			Serial.println("estimulando");
		}	

		
		//xTimerStart(Semg::ledTriggerTimer, 0);
		/*if (Semg::impedanceTooLow()) {
			Session::pause();
		}*/
		if (false) {

		}
		else {
			//Semg::sendTriggerMessage(); ja esta no istrigger 
			//delay(2000);
			Fes::fesLoop();
			
			if(!Fes::emergency_stop) {
				Session::status.complete_stimuli_amount++;
				//Session::sendSessionStatus();
			} else {
				DynamicJsonDocument *message_document = new DynamicJsonDocument(JSON_OBJECT_SIZE(2));
				if (!message_document) {
					printDebug("teste tstes");
				}
				(*message_document)[MESSAGE_KEYS::CODE] = EMERGENCY_STOP_MESSAGE;
				(*message_document)[MESSAGE_KEYS::METHOD] = MESSAGE_METHOD::WRITE;

				MessageHandler::sendMessage(message_document);
			}

			Fes::emergency_stop = false;
			
		
			Session::pauseFromSession();
			//delayBetweenStimuli();
		}
	}
}

void Session::resetSessionStatus(bool session_starting) {
	ESP_LOGI(TAG_SESSION, "Resetting session status");

	SessionStatus::complete_stimuli_amount = 0;
	SessionStatus::interrupted_stimuli_amount = 0;
	SessionStatus::paused = !session_starting;
	SessionStatus::ongoing = session_starting;
	SessionStatus::time_of_last_trigger = 0;
	Fes::emergency_stop = false;
	Session::stop();
}