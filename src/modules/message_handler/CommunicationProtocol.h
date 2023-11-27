#ifndef COMMUNICATION_PROTOCOL
#define COMMUNICATION_PROTOCOL

namespace SESSION_COMMANDS {
    const int START = 2;
    const int STOP = 3;
    const int PAUSE = 4;
    const int RESUME = 5;
    const int SINGLE_STIMULUS = 6;
    const int PARAMETERS = 7;
    const int STATUS = 8;
};

namespace MESSAGE_METHOD {
    const int READ = 'r';
    const int WRITE = 'w';
    const int EXECUTE = 'x';
    const int ACK = 'a';
}

namespace MESSAGE_KEYS {
    const char ANGLE[] = "a";
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
    };
    namespace status {
        const char COMPLETE_STIMULI_AMOUNT[] = "csa";
        const char INTERRUPTED_STIMULI_AMOUNT[] = "isa";
        const char TIME_OF_LAST_TRIGGER[] = "tlt";
        const char SESSION_DURATION[] = "sd";
    }
}

#define GYROSCOPE_MESSAGE 1
#define MESSAGE_CODE_TRIGGER 9
#define EMERGENCY_STOP_MESSAGE 10
#define NE_HANDSHAKE 0

#endif