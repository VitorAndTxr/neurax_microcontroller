#include <iostream>
#include <string>
#include "Debug.h"

void printDebug(const std::string& inputString)
{
    if (DEBUG) {
        std::cout << inputString << std::endl;
    }
}