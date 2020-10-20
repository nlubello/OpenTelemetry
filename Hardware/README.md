# Hardware section

Here is possible to add any hardware related document like schematics, PCB design and BOM.

The required hardware is divide in 2 different sections:

    - [Veichle Board](#veichle-board)
    - [Server Board](#server-board)

# Veichle Board

Currently the application runs on following boards:

|Board|MCU Name|Status|
|-----|-----|-----|
|ESP32|LOLIN32|Development|

## Sensors

|Type|Code|Protocol|Connection|Purpose|
|-----|-----|-----|------|----|
|GPS|MAX M8Q|UART|Pins 16, 17|Track position, speed and get a very accurate time value|
|Termocouple|MAX 6675|SPI|Pins 19, 23, 5| Get exhaust temperature in the range of 500°C-600°C|
|Hall sensor|KY-024|ADC|Pin 36|Throttle position|
|Hall sensor|KY-024|ADC|Pin 39|Brake position|
|Hall sensor|KY-024|ADC|Pin ???|Steering position|
|Hall sensor|KY-024|ADC|Pin ???|Lap time detection|

## Display

|Brand|Model|Size|Protocol|Connection|
|-----|-----|-----|------|----|
|Nextion|[NX4024T032](https://nextion.tech/datasheets/nx4024t032/)|3.2"|UART|Pins 0, 4|
