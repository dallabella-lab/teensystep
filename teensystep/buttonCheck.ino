

void buttonCheck() {
  
  //**************************************************************
  // CHANGES TO IMPLEMENT BUTTON AND LED
  // read the state of the switch button into a local variable:
  int reading = digitalRead(buttonPin);
  
  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:
  
  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
  
    // if the button state has changed:
//    if (reading != buttonState) {
    if (reading == HIGH) {
      buttonState = reading;
      if (buttonActive == false) {
        buttonActive = true;
        buttonTimer = millis();        
      }
      if ((millis() - buttonTimer > longPressTime) && (longPressActive == false)) {
        longPressActive = true;
        configurationMode = !configurationMode;
        if (configurationMode == true) {buttonPushCounter = 0;}
        Serial.print("Configuration mode is: "); Serial.println(configurationMode);
        teensyLedState = !teensyLedState;
        digitalWrite(teensyLedPin, teensyLedState);
      }
    }
    else {
      if (buttonActive == true) { 
        if (longPressActive == true ) {
          longPressActive = false;
        }
        else if (configurationMode == true) {
          configArrayCounter++;
          Serial.print("configArrayCounter:  "); Serial.println(configArrayCounter);
          buttonPushCounter++;
          Serial.println("on");
          Serial.print("number of button pushes:  "); Serial.println(configArrayCounter);
          configurationMode = !configurationMode;
          teensyLedState = !teensyLedState;
          digitalWrite(teensyLedPin, teensyLedState);  
        }
        if (configurationMode == false) {
          ledState = !ledState;
          if (ledState == HIGH){
//            send_config_to_sdCard();
//          send_header_to_sdCard();
            randNumberOfStrides = random(3);
            switch (randNumberOfStrides) {
              case 0:
                endOfConstantCadence = 14;
                break;
              case 1:
                endOfConstantCadence = 16;
                break;
              case 2:
                endOfConstantCadence = 18;
                break;  
            }

            if (configArrayCounter < numberOfTrials){
              rampSlope = configArray[configArrayCounter][2]/configArray[configArrayCounter][3]/100; //takes 4th element of trial config info.
              Serial.print("Ramp Slope is: "); Serial.println(rampSlope,4);
              sound_volume = configArray[configArrayCounter][4];
              Serial.print("Sound Volume is: "); Serial.println(sound_volume,4);
              audioShield.volume(sound_volume); //set sound_volume to default value
            }

            
//            switch (buttonPushCounter) {
//              case 1:
//                //cadence increasing.
//                rampSlope = 0.005;
//                sound_volume = 0.55;
//                audioShield.volume(sound_volume); //set sound_volume to default value
//                break;
//              case 2:
//                //cadence decreasing.
//                rampSlope = -0.005;
//                sound_volume = 0.55;
//                audioShield.volume(sound_volume); //set sound_volume to default value
//                break;
//              case 3:
//                //cadence constant.
//                rampSlope  = 0.0;
//                sound_volume = 0.55;
//                audioShield.volume(sound_volume); //set sound_volume to default value
//                break;
//              case 4:
//                //audio off and no acceleration
//                rampSlope  = 0.0;
//                sound_volume = 0.0;
//                audioShield.volume(sound_volume); //set sound_volume = 0.0
//                break;                
////              default:
////                // if nothing else matches, do the default
////                // default is optional
////                break;
//            }

            // Reset some of the other configuration parameters
            missed_frames           = 0;
            metronome_clicks_played = 0;
            msg_number              = 0; // start messages from zero again
            msg_number_array        = 0;
            msg_number_buffer       = 0;
            clickCounter            = 8; // reset the click countdown.
            cadence                 = 0.0; // reset ioi if we went into configuration mode.
            ramp_ioi                = 0.0;
            tapCounter              = 0;
            
            // Switch system to active mode
  
            //Writing to the SD Card
            char msg[50] = "";
            sprintf(msg, "# Start signal received at t = %lu, randomCadence = %lu\n", current_t, endOfConstantCadence);
            write_to_sdCard(msg);
      
            // Compute when this trial will end
    //          trial_end_t = 60000; // The trial will last 1 minute
            // Compute when this trial will end
            trial_end_t = current_t;
    //          if (metronome) {
    //            trial_end_t += (metronome_nclicks_predelay+metronome_nclicks+1)*metronome_interval; // the +1 here is because from the start moment we will wait one metronome period until we actually start registering
    //          }
    //          trial_end_t   += (ncontinuation_clicks*metronome_interval);
            
            active        = true;
            running_trial = true;
//            Serial.println("Experiment active");
    
            //start the burst of PWM signal
            analogWrite(4,80);
            burst_flag = true;
            prevBurst_t = current_t;
            burst_onset_t = prevBurst_t;
      
//            next_feedback_t  = 0; // ensure that nothing is scheduled to happen any time soon
//            next_metronome_t = 0;
//            
            /* Okay, if we are playing a metronome then let's determine when to start. */
    //          if (metronome) {
    //            next_metronome_t = current_t + metronome_interval;
    //          }
          }
    
          if (ledState == LOW){
    //          Serial.print("# Stop signal received at t=");
    //          Serial.print(current_t);
    //          Serial.print("\n");
    //    
            //Writing to the SD Card
            char msg[50] = "";
            sprintf(msg, "# Stop signal received at t = %lu\n", current_t);
            write_to_sdCard(msg);
            write_bigDataArray_to_sdCard();
            write_bigDataBuffer_to_sdCard();
            active = false; // Put our activity on hold          
          }          
        }
        buttonActive = false;
      }
    }
  }
  
  // set the LED:
  digitalWrite(ledPin, ledState);
  
  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
  //***********************************************************//
}
