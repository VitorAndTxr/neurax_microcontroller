# neurax_microcontroller

![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Espressif](https://img.shields.io/badge/espressif-E7352C.svg?style=for-the-badge&logo=espressif&logoColor=white)

This is the firmware for an electrostimulation device. Check out the [project page here](https://dynamic-vacuum-96a.notion.site/NeuraEstimulator-Blog-5549a27e7c814812b0851a2f0c69d579?pvs=4).

## Configuration values

Configuration values are set in the [platformio.ini](./platformio.ini) file.
Available configurations are:

| General configurations                  | Flag Value    | Explanation     |
|-----------------------------------------|---------------|-----------------|
| SDA_PIN                                 | GPIO21        |I2C SDA pin in ESP32. |
| SCL_PIN                                 | GPIO22        |I2C SCL pin in ESP32. |
| ADC_I2C_ADDR                            | 0x48          |I2C address of the ADC. |
| DEBUG                                   | true          |Enables or disables the debug serial prints. |

| Battery configurations                  | Default Value | Explanation     |
|-----------------------------------------|---------------|-----------------|
| STIMULI_BATTERY_INPUT_PIN               | 3             |The ADC pin to which the stimulation batteries are connected in order to be monitored.|
| MAIN_BATTERY_INPUT_PIN                  | 2             |The ADC pin to which the main batteries are connected in order to be monitored.|
| STIMULI_BATTERY_THRESHOLD               | 3.0           |Low battery voltage threshold (after attenuation)|
| MAIN_BATTERY_THRESHOLD                  | 3.0           |Low battery voltage threshold (after attenuation)|

| LEDs configurations                     | Default Value | Explanation     |
|-----------------------------------------|---------------|-----------------|
| LED_PIN_TRIGGER                         | GPIO25        |                 |
| LED_PIN_FES                             | GPIO26        |                 |
| LED_PIN_POWER                           | GPIO27        |                 |

| FES configurations                      | Default Value | Explanation     |
|-----------------------------------------|---------------|-----------------|
| FES_MODULE_ENABLE                       | true          |Define whether the stimulation is enabled or not. May be disabled for testing purposes. |
| H_BRIDGE_INPUT_1                        | 32            |                 |
| H_BRIDGE_INPUT_2                        | 33            |                 |
| POTENTIOMETER_PIN_INCREMENT             | 25            |                 |
| POTENTIOMETER_PIN_UP_DOWN               | 26            |                 |
| POTENTIOMETER_PIN_CS                    | 27            |                 |
| DEFAULT_POTENTIOMETER_STEPS             | 1             |Amount of increment steps the digital potentiometer should take every time a increment (or decrement) function is called.|
| MAXIMUM_POTENTIOMETER_STEPS             | 100           |                 |
| DEFAULT_STIMULI_DURATION                | 0             | Total duration of FES stimulation in seconds. |
| DEFAULT_PULSE_WIDTH                     | 0             |                 |
| DEFAULT_FREQUENCY                       | 0             |                 |


| sEMG configurations                     | Default Value | Explanation     |
|-----------------------------------------|---------------|-----------------|
| SEMG_ADC_PIN                            | 0             | The ADC pin to which the sEMG module's output is connected. |
| SEMG_ENABLE_PIN                         | 18            | The ESP32 pin to which the AD8232 sEMG module Enable pin is connected.|
| SEMG_FILTER_LOW_CUTOFF_FREQUENCY        | 10.0          |                 |
| SEMG_FILTER_HIGH_CUTOFF_FREQUENCY       | 40.0          |                 |
| SEMG_SAMPLING_TIME                      | 0.01          |                 |
| SEMG_DEFAULT_GAIN                       | 100           |                 |
| SEMG_DIFFICULTY_DEFAULT                 | 16            |                 |
| SEMG_DIFFICULTY_MINIMUM                 | 1             |                 |
| SEMG_DIFFICULTY_MAXIMUM                 | 100           |                 |
| SEMG_DIFFICULTY_INCREMENT               | 1             |                 |
| SEMG_SAMPLES_PER_VALUE                  | 50            |                 |
| SEMG_SAMPLES_PER_AVERAGE                | 10            |                 |
| SEMG_LOW_IMPEDANCE_THRESHOLD            | 10            | Voltage above which the impedance is considered too low. Value in Volts.|


# Libraries
Besides FreeRTOS, the [Filters library](https://dynamic-vacuum-96a.notion.site/NeuraEstimulator-Blog-5549a27e7c814812b0851a2f0c69d579?pvs=4) was also used.
