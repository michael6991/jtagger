# Arduino JTAG/IEEE-1149.1 (1990) Standard Driver for Low Level Purposes

## Brief
Simple low level JTAG driver implemented on the Arduino platform.
Controlled by a python3 host via the Serial0 UART interface of the Arduino.

## Requirements
* pyserial3.5
* To build and flash: Use Arduino IDE that supports the DUE platform

## Notice
* Cannot be used on Arduino-Uno because it has not enough SRAM for the program to run.
* Use a different platform with more than 2KBytes of SRAM. (Use: Mega, Due ...)
* Use logic shifter

# Future Work and Features
* JTAG / IEEE-1149.1 pinout detection --> Jtagulator style (or JTAGEnum)
* ARM SWD pinout detection            --> Jtagulator style (note 38.3.1 in dm00031020 doc)
* UART pinout detection               --> Jtagulator style
* Find a way to detect multiple devices in chain and work seperately on each one
* Option to insert ir,dr to a specific device in chain
* Set verbosity options
* Define a "perror" function
* Utilize TRST with JTAGScan
* Utilize RTCK line for TI devices ?

## Build Notes
``` prepare build system ```
Finding Arduino's Toolchain Paths
Arduino IDE stores toolchains in:
* Linux: ~/.arduino15/packages/
* macOS: ~/Library/Arduino15/packages/
* Windows: %APPDATA%\Arduino15\packages\

### or just install the arduino-cli to mangage platforms and toolchain
* https://arduino.github.io/arduino-cli/ - this will install the necessary 
tools to build and flash for your specific board. In my case SAM Due board.

git clone https://github.com/a9183756-gh/Arduino-CMake-Toolchain.git