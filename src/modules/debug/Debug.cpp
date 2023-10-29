#include <iostream>
#include <string>
#include "debug.h"

void printDebug(const std::string& inputString)
{
    if (DEBUG) {
        std::cout << inputString << std::endl;
    }
}