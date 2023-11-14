#include <iostream>
#include <string>
#include "Debug.h"


void printDebug(String inputString)
{
    if (DEBUG) {
		Serial.println(inputString);
	}
}

/* void printWarn(String& inputString) {
	//setTerminalColor();
	printDebug(RED);
	printDebug(inputString);
	printDebug(RESET_TERMINAL_COLOR);
}

void printMessageAlert(String& inputString) {
	printDebug(BLUE);
	printDebug(inputString);
	printDebug(RESET_TERMINAL_COLOR);
} */

