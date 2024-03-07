# Teensy Step

A framework for implementing sensorimotor synchronisation experiments. The framework is based on Teensy 3.2 and the Audio Adaptor Board for Teensy 3.x. The code reported here will allow the Teensy to record the onset of steps and deliver metronome click sounds over heaphones adapted to the initial cadence of the steps. Data is saved on the onboard SD Card for offline analysis. 

![alt text](misc/Equipment_TeensyStep.jpg "Setup example")

## Requirements

### Hardware
* [Teensy 3.2](https://www.pjrc.com/store/teensy32.html) 
* [Audio Adaptor](https://www.pjrc.com/store/teensy3_audio.html) for Teensy 3.2
* See the section [Building the circuit](#building-the-circuit) for detailed information

### Development software
The following software is required for uploading the code to Teensy.

* [Arduino IDE](https://www.arduino.cc/en/Main/Software). Code developed with Arduino 1.8.13.
* [Teensyduino extension for Arduino IDE](https://www.pjrc.com/teensy/teensyduino.html). Code developed using Teensyduino 1.53.


### Circuit schematic
Circuit diagram included here.

![wiring](misc/wiringTeensyStep.png). 


### Installation (needs to be done only once)


1. Download the [Arduino IDE](https://www.arduino.cc/en/Main/Software) - (Teensyduino only works with some specific versions of Arduino IDE).

2. Download [Teensyduino](https://www.pjrc.com/teensy/td_download.html) and run the installer, pointing it to where you have installed the Arduino code. 

3. Download or clone this repository (i.e. `git clone https://github.com/dallabella-lab/teensystep`).


## Close-ups

![custom shield](misc/TopBoard.png "Custom shield for the Teensy 3.2")

---

![side view](misc/BottomBoard.png "Teensy 3.2 with Audio board")


## Usage

Once the prototype has been manufactured it can be attached to the participant's ankle to record their steps as shown in the picture below. The device should be tight enough to prevent movement during walking, but not too tight to cause discomfort or restrict the participant's movement. The FSR should be attached to a sole and placed under the heel, in the participant's shoe.

<img src="misc/Equipment_participant.jpg" alt="equipment" align="center" width="500"/>

## How to conduct the Ramp protocole?

# Before the exeperiment

Before the experiment, prepare a txt file called "CONFIG" and save it on the SD card. This file contains the trials parameters (order of trials, slope of the tempo change, number of steps to complete the trial).

![CONFIG detail](misc/trial_configuration.png "Details of the trial parameters")

In this study, there were 4 trial types with tempo change relative to the initial cadence of the participant: 1. 20% acceleration; 2. 20% deceleration; 3. no change; 4. no change with no sound (i.e. baseline).

# How to start a trial?
After inserting the SD card, and connecting the battery and FSR to the board, start the trial by pressing once (short press) on the buttonpress switch (see the Custom shiel close-up above).
Additional short press will restart the same trial. To start the next trial type (see CONFIG detail), a long press (i.e. more than one second) will turn the Teensy LED on and select the following line on the CONFIG file. Then, you can click again on the same button (short press) to start the trial.
After a short press, the LED of the custom board is on, indicating the trial is currently running which helps monitoring the experiment.

## Sampling frequency

The sampling frequency of Teensystep is 1KHz.

## References

The code for this project started as a modification of TeensyTap by Floris van Vugt, Sept 2019 [https://github.com/florisvanvugt/teensytap](https://github.com/florisvanvugt/teensytap)



