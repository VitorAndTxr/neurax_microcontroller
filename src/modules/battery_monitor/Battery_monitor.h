#ifndef BATTERY_MONITOR
#define BATTERY_MONITOR

#include "../battery/Battery.h"

class Battery_monitor
{
private:
    static Battery stimuli_battery;
    static Battery main_battery;
public:
    Battery_monitor();
    ~Battery_monitor();
};

Battery Battery_monitor::stimuli_battery(STIMULI_BATTERY_INPUT_PIN, STIMULI_BATTERY_THRESHOLD);

Battery Battery_monitor::main_battery(MAIN_BATTERY_INPUT_PIN, MAIN_BATTERY_THRESHOLD);

Battery_monitor::Battery_monitor()
{
    //tarefa para monitorar ambas as baterias
    // provavelment com prioridade mais baixa
    
}

Battery_monitor::~Battery_monitor()
{
}



#endif