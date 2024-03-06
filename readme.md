# Teensy Step

A framework for running sensorimotor synchronisation experiment. The framework is based on Teensy and the Audio Adapter, which are inexpensive and readily available for purchase at many retailers internationally. The code provided here will allow the Teensy to record onset of steps and deliver auditory stimulus over headphones and simultaneously present metronome click sounds. Data is saved on the onboard SDCard for offline analysis. 

![alt text](misc/Equipment_TeensyStep.jpg "Setup example")

## Requirements

### Hardware
* [Teensy 3.2](https://www.pjrc.com/store/teensy32.html) (may also work with later versions)
* [Audio Adapter](https://www.pjrc.com/store/teensy3_audio.html) for Teensy 3.2.
* FSR sensor (Force-sensitive resistor)
* Break-away straight headers (to connect Teensy with the Audio Adapter)
* A few wires and a resistor
* Tools: soldering iron and tin

### Development software
The following software is required for uploading the code to Teensy.

* [Arduino IDE](https://www.arduino.cc/en/Main/Software). Code developed with Arduino 1.8.13.
* [Teensyduino extension for Arduino IDE](https://www.pjrc.com/teensy/teensyduino.html). Teensyduino is basically a set of tools that allow you to use the Arduino development environment to make code for your Teensy. Currently using Teensyduino 1.53.


### Building the circuit
Refer to the circuit diagram included here.

1. Solder the Teensy and the Audio Board together. You have to solder only the pins marked in the wiring diagram with letter A (to save time).

2. Solder the wires to the resistor and FSR as indicated. 

![wiring](misc/wiringTeensyStep.png). 


### Installation (needs to be done only once)


1. Download the [Arduino IDE](https://www.arduino.cc/en/Main/Software) - **important** make sure you download version **1.8.13** (Teensyduino only works with some specific versions of Arduino IDE).

2. Download [Teensyduino](https://www.pjrc.com/teensy/td_download.html) and run the installer, pointing it to where you have installed the Arduino code. If you're under Linux, make sure you download the udev rules as indicated on the site above.

3. Download or clone this repository (i.e. `git clone https://gitlab.com/sdblab/teensystep`).


## Close-ups

![custom shield](misc/TopBoard.png "Custom shield for the Teensy 3.2")

---

![side view](misc/BottomBoard.png "Teensy 3.2 with Audio board")


## Usage

Once the prototype has been manufactured following the steps above and is ready to be used, it can be attached to the participant's ankle to record their steps as shown in the picture below. The device should be tight enough to prevent movement during walking, but not too tight to cause discomfort or restrict the participant's movement.

<img src="misc/Equipment_participant.jpg" alt="equipment" align="center" width="500"/>

## Sampling frequency

The sampling frequency of Teensystep is 1KHz.

## References

The code for this project started as a modification of TeensyTap by Floris van Vugt, Sept 2019 [https://github.com/florisvanvugt/teensytap](https://github.com/florisvanvugt/teensytap)



