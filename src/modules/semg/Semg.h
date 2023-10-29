#ifndef SEMG_MODULE
#define SEMG_MODULE
#include "../SemgFilter/SemgFilter.h"

class Semg
{
private:
    SemgFilter* filter;
    int pin;
    float last_value; 
    
    static float filtered_value[50];
    static float mes[5];
    static float values[2];

    static float gain;
    static float difficulty;

    static float threshold;

    static float mes_a[2];
    static float mes_b[2];
    float voltage;
public:
    Semg(int pin);
    ~Semg();
    void increaseDifficulty(int increment);
    void decreaseDifficulty(int decrement);
    bool isTrigger();
    void updateMovingThreshold();
    bool impedanceTooLow();
    float readSensor();
    void acquire();
    void print();
};





#endif