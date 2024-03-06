
void button2Check(){
  // read the state of the switch button into a local variable:
  int reading2 = digitalRead(buttonResponsePin);
  
//  Serial.println(reading);
//  delay(100);

  // If the switch changed, due to noise or pressing:
  if (reading2 != lastButton2State) {
    // reset the debouncing timer
    lastDebounceTime2 = millis();
  }

  if ((millis() - lastDebounceTime2) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
    // save the reading. Next time through the loop, it'll be the lastButtonState:

    if (reading2 == HIGH && button2State == LOW) {
      button2State = HIGH;
      send_response_to_bigDataArray();
    }
    else if (reading2 == LOW && button2State == HIGH){
      button2State = LOW;
    }
  }
  
  lastButton2State = reading2;

}
