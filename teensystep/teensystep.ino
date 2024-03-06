

/*
  Teensy Step v2

  A Teensy 3.2 interface that is designed to
  - capture onset of stes from a FSR installed in sole (see pictures).
  - deliver auditory feedback, possibly delayed, at every tap
  - play a metronome sound.


  Floris van Vugt, Sept 2017
  Based on TapArduino


  This is code designed to work with Teensyduino
  (i.e. the Arduino IDE software used to generate code you can run on Teensy)

  Code modified in late 2020 in BRAMS by Alex Nieva & Agnès Zagala to detect step synchronization.

  January 2023 - Update to read trials from csv file.
  Reading CSV file based on https://medium.com/@pranavansp/reading-csv-file-data-from-sd-card-with-an-arduino-8c8276758963

 */


#include <SD.h>
#include <SPI.h>
#include <Audio.h>

#include <ADC.h>
#include <ADC_util.h>
#include <CircularBuffer.h>


// Load the samples that we will play for taps and metronome clicks, respectively
#include "AudioSampleTap.h"
#include "AudioSampleMetronome.h"
#include "AudioSampleEndsignal.h"

#include "DeviceID_custom.h"

ADC *adc = new ADC();; // adc object

#define WIDTH_BIGDATAARRAY 5
#define HEIGHT_BIGDATAARRAY 120
#define WIDTH_BIGDATABUFFER 400
#define HEIGHT_BIGDATABUFFER 25

CircularBuffer<int, WIDTH_BIGDATABUFFER> buffer; //cirular buffer
CircularBuffer<long, 8> beatBuffer; //Buffer to detect the cadence of participant.
using index_t = decltype(beatBuffer)::index_t;
index_t discardedStrides = 3;
int clickCounter = 8;
long next_click_t;
int tapCounter = 0;
float cadence = 0.0;
float ramp_ioi   = 0.00; // the proportion of change of the metronome IOI during the ramp phase
float rampSlope = 0.005;
long randNumberOfStrides;
long endOfConstantCadence;

/*
  Setting up infrastructure for capturing taps (from a connected FSR)
*/

int bigDataArray[HEIGHT_BIGDATAARRAY][WIDTH_BIGDATAARRAY] = {0};
int bigDataBuffer[HEIGHT_BIGDATABUFFER][WIDTH_BIGDATABUFFER] = {0};

#define TAP 1
#define CLICK 2
#define BURST 3
#define DOWNSAMPLER 2
#define RESPONSE 4
int bufferUpdater = DOWNSAMPLER;

boolean active = false; // Whether the tap capturing & metronome and all is currently active
boolean prev_active = false; // Whether we were active on the previous loop iteration

int fsrAnalogPin = A3; // FSR is connected to analog 3 (A3)
int fsrReading;      // the analog reading from the FSR resistor divider
int ledPin = A6;      // LED to show system ON/OFF
int buttonPin = 5;    // Button to start/stop system
int buttonResponsePin = 8; // Button for participant response.
const int teensyLedPin = 13;  //Teensy LED pin to show config mode. 
int teensyLedState = LOW;

//Button debounce variables
int buttonState;
int ledState = LOW;
int lastButtonState = LOW;
int button2State;
int lastButton2State = LOW;
boolean sentResponse = LOW;

unsigned long lastDebounceTime = 0;
unsigned long lastDebounceTime2 = 0;
unsigned long debounceDelay = 50;

//Button timer variables
unsigned long buttonTimer = 0;
long longPressTime = 1000;

boolean buttonActive = false;
boolean longPressActive = false;
boolean configurationMode = false;
int buttonPushCounter = 1;   // counter for the number of button presses

// Definitions for the SD card
const int chipSelect = 10;
#define DATAFILE "DATALOG.TXT"
#define DATAFRAMES "WAVES.TXT"
#define DATA1 "DATA1.TXT"

// SD card setup stuff
Sd2Card card;
SdVolume volume;
File dataFile0;
File dataFile1;
File dataFile2;
boolean status;
int type;
float size;

/* CSV File Reading */
File file;
//int SC = 53;  //SC - Pin 53 Arduino Mega
char location;

bool readLine(File &f, char* line, size_t maxLen) {
  for (size_t n = 0; n < maxLen; n++) {
    int c = f.read();
    if ( c < 0 && n == 0) return false;  // EOF
    if (c < 0 || c == '\n') {
      line[n] = 0;
      return true;
    }
    line[n] = c;
  }
  return false; // line too long
}

//bool readVals(long* v1, long* v2, long* v3, long* v4, String* loc,String* loc2) {
bool readVals(long* v1, long* v2, long* v3, long* v4, float* v5) {
  
  char line[200], *ptr, *str;
  if (!readLine(file, line, sizeof(line))) {
    return false;  // EOF or too long
  }
  *v1 = strtol(line, &ptr, 10);
  if (ptr == line) return false;  // bad number if equal
  while (*ptr) {
    if (*ptr++ == ',') break;
  }
  *v2 = strtol(ptr, &str, 10);
  while (*ptr) {
    if (*ptr++ == ',') break;
  }
  *v3 = strtol(ptr, &str, 10);
  while (*ptr) {
    if (*ptr++ == ',') break;
  }
  *v4 = strtol(ptr, &str, 10);
  while (*ptr) {
    if (*ptr++ == ',') break;
  }
  *v5 = strtod(ptr, &str);
  while (*ptr) {
    if (*ptr++ == ',') break;
  }
//  String a = strtok_r(ptr, ",", &str);
//  String first(str);
//  *loc = first;
//  String let(a);
//  *loc2 = let;
  
  return str != ptr;  // true if number found
}
/* Close CSV File Reading */

// Array to store the data read from SD Card regarding trials.
#define numberOfTrials 36
float configArray[numberOfTrials][5]={0.0};
int configArrayCounter = 0;

// For interpreting taps
int tap_onset_threshold    = 1750; // the FSR reading threshold necessary to flag a tap onset
int tap_offset_threshold   = 500; // the FSR reading threshold necessary to flag a tap offset
int min_tap_on_duration    = 350; // the minimum duration of a tap (in ms), this prevents double taps
int min_tap_off_duration   = 300; // the minimum time between offset of one tap and the onset of the next taps, again this prevents double taps


int tap_phase = 0; // The current tap phase, 0 = tap off (i.e. we are in the inter-tap-interval), 1 = tap on (we are within a tap)


unsigned long current_t            = 0; // the current time (in ms)
unsigned long prev_t               = 0; // the time stamp at the previous iteration (used to ensure correct loop time)
unsigned long next_event_embargo_t = 0; // the time when the next event is allowed to happen
unsigned long trial_end_t          = 0; // the time that this trial will end
unsigned long prevBurst_t          = 0; // the time stamp at the previous iteration (used to ensure correct loop time)
unsigned long burst_onset_t        = 0; // 
unsigned long burst_offset_t       = 0; //

unsigned long tap_onset_t = 0;  // the onset time of the current tap
unsigned long tap_offset_t = 0; // the offset time of the current tap
int           tap_max_force = 0; // the maximum force reading during the current tap
unsigned long tap_max_force_t = 0; // the time at which the maximum tap force was experienced


int missed_frames = 0; // ideally our script should read the FSR every millisecond. we use this variable to check whether it may have skipped a millisecond

int metronome_interval = 600; // Time between metronome clicks

unsigned long next_metronome_t            = 0; // the time at which we should play the next metronome beat
unsigned long next_feedback_t             = 0; // the time at which the next tap sound should occur


int metronome_clicks_played = 0; // how many metronome clicks we have played (used to keep track and quit)

boolean running_trial = false; // whether we are currently running the trial

// parameters of the burst, from arduino to delsys
int trigger_frequency = 5000; // the frequency of the analog trigger sent in ms
int burst_time = 100;
//unsigned long burst_timer = 0;
bool burst_flag = false;


/*
  Information pertaining to the trial we are currently running
*/

// Hardcoded values that come from the Python GUI

int auditory_feedback          = 1; // whether we present a tone at every tap
int auditory_feedback_delay    = 0; // the delay between the tap and the to-be-presented feedback
int metronome                  = 1; // whether to present a metronome sound
int metronome_nclicks_predelay = 10; // how many clicks of the metronome to occur before we switch on the delay (if any)
int metronome_nclicks          = 20; // how many clicks of the metronome to present on this trial
int ncontinuation_clicks       = 20; // how many continuation clicks to present after the metronome stops


/*
  Setting up the audio
*/

float sound_volume = .55; // the volume

// Create the Audio components.
// We create two sound memories so that we can play two sounds simultaneously
AudioPlayMemory    sound0;
AudioPlayMemory    sound1;
AudioMixer4        mix1;   // one four-channel mixer (we'll only use two channels)
AudioOutputI2S     headphones;

// Create Audio connections between the components
AudioConnection c1(sound0, 0, mix1, 0);
AudioConnection c2(sound1, 0, mix1, 1);
AudioConnection c3(mix1, 0, headphones, 0);
AudioConnection c4(mix1, 0, headphones, 1); // We connect mix1 to headphones twice so that the sound goes to both ears

// Create an object to control the audio shield.
AudioControlSGTL5000 audioShield;


/*
  Serial communication stuff
*/


int msg_number = 0; // keep track of how many messages we have sent over the serial interface (to be able to track down possible missing messages)
int msg_number_array = 0; //
int msg_number_buffer = 0; //


const int MESSAGE_START   = 77;   // Signal to the Teensy to start
const int MESSAGE_CONFIG  = 88;   // Signal to the Teensy that we are going to send a trial configuration
const int MESSAGE_STOP    = 55;   // Signal to the Teensy to stop

const int CONFIG_LENGTH = 6*4; /* Defines the length of the configuration packet */
  

void setup(void) {
  Serial.print("TeensyStep starting...\n");

  /* This function will be executed once when we power up the Teensy */
  SPI.setMOSI(7);  // Audio shield has MOSI on pin 7
  SPI.setSCK(14);  // Audio shield has SCK on pin 14
  

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(10);

  // turn on the output
  audioShield.enable();
  audioShield.volume(sound_volume);

  // reduce the gain on mixer channels, so more than 1
  // sound can play simultaneously without clipping
  mix1.gain(0, 0.5);
  mix1.gain(1, 0.5);

  ///// ADC0 ////
  adc->adc0->setAveraging(0); // set number of averages
  adc->adc0->setResolution(13); // set bits of resolution
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::MED_SPEED); // change the conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED); // change the sampling speed

  // First, detect the card
  status = card.init(SPI_FULL_SPEED, chipSelect);
  if (status) {
    Serial.println("SD card is connected :-)");
  } else {
    Serial.println("SD card is not connected or unusable :-(");
    return;
  }

  type = card.type();
  if (type == SD_CARD_TYPE_SD1 || type == SD_CARD_TYPE_SD2) {
    Serial.println("Card type is SD");
  } else if (type == SD_CARD_TYPE_SDHC) {
    Serial.println("Card type is SDHC");
  } else {
    Serial.println("Card is an unknown type (maybe SDXC?)");
  }

  // Then look at the file system and print its capacity
  status = volume.init(card);
  if (!status) {
    Serial.println("Unable to access the filesystem on this card. :-(");
    return;
  }

  size = volume.blocksPerCluster() * volume.clusterCount();
  size = size * (512.0 / 1e6); // convert blocks to millions of bytes
  Serial.print("File system space is ");
  Serial.print(size);
  Serial.println(" Mbytes.");

  // Initialize the SD card
  while (!(SD.begin(chipSelect))) {
    // stop here if no SD card, but print a message
   // while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
   // }
  }

//  file = SD.open("gps.CSV", FILE_READ);
  file = SD.open("CONFIG.txt", FILE_READ);
  if (!file) {
    Serial.println("open error");
    return;
  }

  //Reading the values of config in the sd card into array. Rough. To be optimized.
  long x, y, z, j;
  float k;
  int readingCount = 0;
//  String loc,loc2;
  while (readVals(&x, &y, &z, &j, &k)) {
    
    //First 5 datatype variables 
    Serial.println(x);
    Serial.println(y);
    Serial.println(z);
    Serial.println(j);
    Serial.println(k);
    configArray[readingCount][0] = x;
    configArray[readingCount][1] = y;
    configArray[readingCount][2] = z;
    configArray[readingCount][3] = j;
    configArray[readingCount][4] = k;
    readingCount++;
//    //Last 2 String type variables
//    Serial.println(loc);
//    Serial.println(loc2);
  } 


//Writing to the SD Card when rebooted (in case of new participant or power loss)
char msg[50] = "";
sprintf(msg, "-----------------------------------");
write_to_sdCard(msg); 

// Button and LED setup
  pinMode(buttonPin, INPUT);
  pinMode(buttonResponsePin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(teensyLedPin, OUTPUT);
  digitalWrite(ledPin, ledState);
  digitalWrite(teensyLedPin, teensyLedState);

//  Serial.print("TeensyTap ready.\n");

  active = false;
// analogWrite pin setup
  pinMode(4,OUTPUT);
  pinMode(A3,INPUT);
  //analogWriteFrequency(4, 100); // PWM send from Teensy pin 4 to delsys analoginput adapter

  //Random number generator seed.
  randomSeed(analogRead(A14));
}

void do_activity() {
  /* This is the usual activity loop */
  
  /* If this is our first loop ever, initialise the time points at which we should start taking action */
  if (prev_t == 0)           { prev_t = current_t; } // To prevent seeming "lost frames"
//  if (next_metronome_t == 0) { next_metronome_t = current_t+metronome_interval; }
  
  if (current_t > prev_t) {
    // Main loop tick (one ms has passed)    
    
    if ((prev_active) && (current_t-prev_t > 1)) {
      // We missed a frame (or more)
      missed_frames += (current_t-prev_t);
    }
    
    
    /*
     * Collect data
     * Now using the ADC library
     */
    fsrReading = adc->adc0->analogRead(fsrAnalogPin);
    //Push value to circular buffer every DOWNSAMPLER samples (using modulo 2 for now)
    if (bufferUpdater % DOWNSAMPLER == 0) {
      buffer.push(fsrReading);
      bufferUpdater--;
    }
    else {
      bufferUpdater--;
      if (bufferUpdater == 0) {
        bufferUpdater = DOWNSAMPLER;
      }
    }    
    //Push value to circular buffer
    //buffer.push(fsrReading);
    //Serial.println(fsrReading);

    /*
     * Process data: has a new tap onset or tap offset occurred?
     */

    if (tap_phase==0) {
      // Currently we are in the tap-off phase (nothing was thouching the FSR)
      
      /* First, check whether actually anything is allowed to happen.
   For example, if a tap just happened then we don't allow another event,
   for example we don't want taps to occur impossibly close (e.g. within a few milliseconds
   we can't realistically have a tap onset and offset).
   Second, check whether this a new tap onset
      */
if ( (current_t > next_event_embargo_t) && (fsrReading>tap_onset_threshold)) {

  // New Tap Onset
  tap_phase = 1; // currently we are in the tap "ON" phase
  tap_onset_t = current_t;
  tapCounter++;
  if (clickCounter > 0) {
    beatBuffer.push(tap_onset_t);
    clickCounter--;  
  }

  if (clickCounter == 0) {
    // the following ensures using the right type for the index variable
//    using index_t = decltype(beatBuffer)::index_t;
    for (index_t i = discardedStrides; i < beatBuffer.size(); i++) { //i = discardedStrides because we don't want the first 4 strides. See variable definition at the top.
      cadence += (beatBuffer[i+1] - beatBuffer[i]);      
      //avg += beatBuffer[i] / (float)beatBuffer.size();
    }
    cadence = cadence/(float)(beatBuffer.size() - discardedStrides - 1);
    ramp_ioi = cadence;
    clickCounter--;
    next_click_t = beatBuffer.last();
  }

  // don't allow an offset immediately; freeze the phase for a little while
  next_event_embargo_t = current_t + min_tap_on_duration;

  // Schedule the next tap feedback time (if we deliver feedback)
//  if (metronome && metronome_clicks_played < metronome_nclicks_predelay) {
//    next_feedback_t = current_t; // if we are in the pre-delay period, let's play the feedback sound immediately.
//  } else {
//    next_feedback_t = current_t + auditory_feedback_delay;
//  }
}
      
    } else if (tap_phase==1) {
      // Currently we are in the tap-on phase (the subject was touching the FSR)
      
      // Check whether the force we are currently reading is greater than the maximum force; if so, update the maximum
//      if (fsrReading>tap_max_force) {
//        tap_max_force_t = current_t;
//        tap_max_force   = fsrReading;
//      }
      
      // Check whether this may be a tap offset
      if ( (current_t > next_event_embargo_t) && (fsrReading<tap_offset_threshold)) {

        // New Tap Offset
        tap_phase = 0; // currently we are in the tap "OFF" phase
        tap_offset_t = current_t;
        
        // don't allow an offset immediately; freeze the phase for a little while
        next_event_embargo_t = current_t + min_tap_off_duration;
      
        // Send data to the computer!
      //  send_tap_to_serial();
//        send_tap_to_sdCard();
        send_tap_to_bigArrayData();
//        send_buffer_to_sdCard();
        if (tapCounter < HEIGHT_BIGDATABUFFER) {
          send_buffer_to_bigDataBuffer();
        }
      
        // Clear information about the tap so that we are ready for the next tap to occur
        tap_onset_t     = 0;
        tap_offset_t    = 0;
//        tap_max_force   = 0;
//        tap_max_force_t = 0;
      }
    }

    if (clickCounter < 1 && (current_t > (next_click_t + ramp_ioi))) {
      // Play metronome click
//      Serial.print("ramp_ioi: "); Serial.println(ramp_ioi);
      sound1.play(AudioSampleMetronome);
//      send_metronome_to_sdCard();
      send_metronome_to_bigDataArray();
      next_click_t = current_t;
      metronome_clicks_played += 1;
      if (metronome_clicks_played == 1) {
        cadence = cadence/2;
        ramp_ioi = cadence;
      }
      if (metronome_clicks_played > endOfConstantCadence){ //20 because we want 10 per leg.
         ramp_ioi = cadence*(1 - (metronome_clicks_played - endOfConstantCadence)*rampSlope);
         if (buttonPushCounter == 1 && (metronome_clicks_played > endOfConstantCadence + 39)){
           sound_volume = 0.55;
           audioShield.volume(sound_volume); //set sound_volume to default value
         }
         if (metronome_clicks_played > endOfConstantCadence + 40){
           running_trial = false;
           active = false;
          //Writing to the SD Card
          char msg[50] = "";
          sprintf(msg, "# Trial completed at t = %lu\n", current_t);
          write_to_sdCard(msg);           
         }
       }
    }       
    
    /* 
     * Deal with the metronome
    */
//    // Is this a time to play a metronome click?
//    if (metronome && (metronome_clicks_played < metronome_nclicks_predelay + metronome_nclicks)) {
//      if (current_t >= next_metronome_t) {
//        // Mark that we have another click played
//        metronome_clicks_played += 1;
//      
//        // Play metronome click
//        sound1.play(AudioSampleMetronome);
//        
//        // And schedule the next upcoming metronome click
//        next_metronome_t += metronome_interval;
//        
//        // Proudly tell the world that we have played the metronome click
//      //  send_metronome_to_serial();
//        send_metronome_to_sdCard();
//      }
//    }
//
//    if (auditory_feedback) {
//      if ((next_feedback_t != 0) && (current_t >= next_feedback_t)) {
//        // Play the auditory feedback (relating to the subject's tap)
//        sound0.play(AudioSampleTap);
//      
//        // Clear the queue, nothing more to play
//        next_feedback_t = 0;
//        
//        // Proudly tell the world that we have played the tap sound
//      //  send_feedback_to_serial();
//        send_feedback_to_sdCard();
//      }
//    }
    
    // Update the "previous" state of variables
    prev_t = current_t;
  }
}

void loop(void) {
  /* This is the main loop function which will be executed ad infinitum */

  current_t = millis(); // get current time (in ms)

  

  if (active) { 
    if (current_t >= (prevBurst_t+trigger_frequency)) {
      analogWrite(4, 80);
      //digitalWrite(4,HIGH);
      burst_flag = true;
      prevBurst_t = current_t;
      burst_onset_t = prevBurst_t;
    }

    if ((burst_flag == true) && (current_t >= (prevBurst_t + burst_time))){
      analogWrite(4,0);
      //digitalWrite(4,LOW);
      burst_flag = false;
      burst_offset_t = current_t;
//      send_burst_to_sdCard();
      send_burst_to_bigDataArray();

    }  
    do_activity(); 
  }
  // Signal for the next loop iteration whether we were active previously.
  // For example, if we weren't active previously then we don't want to count lost frames.
  prev_active = active;


//  if (active && running_trial && (current_t > trial_end_t)) {
//    // Trial has ended (we have completed the number of metronome clicks and continuation clicks)
//
//    // Play another sound to signal to the subject that the trial has ended.
//    sound1.play(AudioSampleEndsignal);
//
//    // Communicate to the computer
////    Serial.print("# Trial completed at t=");
////    Serial.print(current_t);
////    Serial.print("\n");
////    
//    //Writing to the SD Card
//    char msg[50] = "";
//    sprintf(msg, "# Trial completed at t = %lu\n", current_t);
//    write_to_sdCard(msg);
//        
//    running_trial = false;
//  }

  buttonCheck();
  button2Check();

}


void send_config_to_sdCard() {
  /* Sends a dump of the current config to the SD card*/
  char msg[200];
//  msg_number += 1; // This is the next message
//  Serial.print  ("# Device installed ");
//  Serial.println(DEVICE_ID);
  sprintf(msg, "# config AF=%i DELAY=%i METR=%i INTVL=%i NCLICK_PREDELAY=%i NCLICK=%i NCONT=%i\n",
    auditory_feedback,
    auditory_feedback_delay,
    metronome,
    metronome_interval,
    metronome_nclicks_predelay,
    metronome_nclicks,
    ncontinuation_clicks);
    write_to_sdCard(msg);
}


//void send_header_to_sdCard(){
//  char msg[200] = "message_number type onset_t offset_t max_force_t max_force n_missed_frames\n";
//  write_to_sdCard(msg);
//}

void send_tap_to_sdCard(){
  /* Sends information about the current tap to the SD card  */
  char msg[100];
  msg_number += 1; // This is the next message
  sprintf(msg, "%d tap %lu %lu %lu %d %d\n",
    msg_number,
    tap_onset_t,
    tap_offset_t,
    tap_max_force_t,
    tap_max_force,
    missed_frames);
  write_to_sdCard(msg);  
}

void send_tap_to_bigArrayData() {
  msg_number_array += 1;
  bigDataArray[msg_number_array][0] = msg_number_array;
  bigDataArray[msg_number_array][1] = TAP;
  bigDataArray[msg_number_array][2] = tap_onset_t;
  bigDataArray[msg_number_array][3] = tap_offset_t;
  bigDataArray[msg_number_array][4] = missed_frames;
}

//void send_feedback_to_sdCard() {
//  /* Sends information about the current tap to the SD card */
//  char msg[100];
//  msg_number += 1; // This is the next message
//  sprintf(msg, "%d feedback %lu NA NA NA %d\n",
//    msg_number,
//    current_t,
//    missed_frames);
//  write_to_sdCard(msg);
//}

void send_metronome_to_sdCard() {
  /* Sends information about the current tap to the SD card */
  char msg[100];
  msg_number += 1; // This is the next message
  sprintf(msg, "%d click %lu NA NA NA %d\n",
    msg_number,
    current_t,
    missed_frames);
  write_to_sdCard(msg);
}

void send_metronome_to_bigDataArray() {
  msg_number_array += 1;
  bigDataArray[msg_number_array][0] = msg_number_array;
  bigDataArray[msg_number_array][1] = CLICK;
  bigDataArray[msg_number_array][2] = current_t;
  bigDataArray[msg_number_array][4] = missed_frames;
// bigDataArray[msg_number_array][4] = next_click_t;

}

void write_bigDataArray_to_sdCard() {
  dataFile2 = SD.open(DATA1,FILE_WRITE);
  if (dataFile2) {
    for (int i = 0; i < HEIGHT_BIGDATAARRAY; i++) {
      for (int j = 0; j < WIDTH_BIGDATAARRAY; j++) {
//        Serial.print("Big data array: "); Serial.print(i); Serial.println(bigDataArray[i][j]);
        dataFile2.print(bigDataArray[i][j]);
        dataFile2.print(",");
      }
      dataFile2.print("\n");
    }
    dataFile2.close();
  }
  else {
    Serial.println("error opening example.txt");    
  }
}


void send_burst_to_sdCard() {
  /* Sends information about the current tap to the SD card */
  char msg[100];
  msg_number += 1; // This is the next message
  sprintf(msg, "%d burst %lu %lu NA NA %d\n",
    msg_number,
    burst_onset_t,
    burst_offset_t,
    missed_frames);
  write_to_sdCard(msg);
}

void send_burst_to_bigDataArray() {
  msg_number_array += 1;
  bigDataArray[msg_number_array][0] = msg_number_array;
  bigDataArray[msg_number_array][1] = BURST;
  bigDataArray[msg_number_array][2] = burst_onset_t;
  bigDataArray[msg_number_array][3] = burst_offset_t;
  bigDataArray[msg_number_array][4] = missed_frames;  
}

void send_response_to_bigDataArray() {
  msg_number_array += 1;
  bigDataArray[msg_number_array][0] = msg_number_array;
  bigDataArray[msg_number_array][1] = RESPONSE;
  bigDataArray[msg_number_array][2] = current_t - debounceDelay;
//  bigDataArray[msg_number_array][3] = burst_offset_t;
  bigDataArray[msg_number_array][4] = missed_frames;  
}


void write_to_sdCard(char *msg){
  // open the file.
  dataFile0 = SD.open(DATAFILE, FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile0) {
    dataFile0.println(msg);
    dataFile0.close();
    // print to the serial port too:
    //Serial.println(dataString);
  }  
  // if the file isn't open, pop up an error:
  else {
//    Serial.println("error opening datalog.txt");
  }  
}

void send_buffer_to_sdCard() {
  //Serial.println("SD");
  //Serial.print("bufferPrint: "); Serial.println(bufferPrint);
  dataFile1 = SD.open(DATAFRAMES,FILE_WRITE);
  if (dataFile1) {
    while (!buffer.isEmpty()) {
      dataFile1.print(buffer.shift());
      dataFile1.print(",");
    }
    dataFile1.println("X");
    dataFile1.close();    
  }
  else {
    Serial.println("error opening example.txt");    
  }
}

void send_buffer_to_bigDataBuffer() {
  int index = 0;
  while (!buffer.isEmpty()) {
    bigDataBuffer[msg_number_buffer][index] = buffer.shift();
    index++;
  }
  msg_number_buffer += 1;
}

void write_bigDataBuffer_to_sdCard() {
  dataFile1 = SD.open(DATAFRAMES,FILE_WRITE);
  if (dataFile1) {
    for (int i = 0; i < HEIGHT_BIGDATABUFFER; i++) {
      for (int j = 0; j < WIDTH_BIGDATABUFFER; j++) {
//        Serial.print("Big data array: "); Serial.print(i); Serial.println(bigDataArray[i][j]);
        dataFile1.print(bigDataBuffer[i][j]);
        dataFile1.print(",");
      }
      dataFile1.print("\n");
    }
    dataFile1.close();
  }
  else {
    Serial.println("error opening example.txt");    
  }  
}
