#ifndef COMMUNICATION_PROTOCOL
#define COMMUNICATION_PROTOCOL

namespace SESSION_COMMANDS {
    const int START = 2;
    const int STOP = 3;
    const int PAUSE = 4;
    const int RESUME = 5;
    const int SINGLE_STIMULUS = 6;
    const int STATUS = 8;
    const int PARAMETERS = 7;
};

namespace MESSAGE_METHOD {
    const char READ = 'r';
    const char WRITE = 'w';
    const char EXECUTE = 'x';
    const char ACK = 'a';
}

namespace MESSAGE_KEYS {
	const char CODE[] = "cd";
    const char METHOD[] = "mt";
    const char BODY[] = "bd";
    const char PARAMETERS[] = "parameters";
    const char STATUS[] = "status";
	namespace parameters {
    	const char AMPLITUDE[] = "a";
    	const char FREQUENCY[] = "f";
    	const char PULSE_WIDTH[] = "pw";
    	const char DIFFICULTY[] = "df";
    	const char STIMULI_DURATION[] = "pd";
	}
}

#define GYROSCOPE_MESSAGE 1
#define SESSION_MESSAGE 2
#define NE_HANDSHAKE 0
#define NE_ACK 9


#endif