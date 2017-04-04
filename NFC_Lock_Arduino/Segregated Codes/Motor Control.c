// Unlock & HW Setup
bool lockHWSetup()
{
  // Motor Pins
  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);

  // Motor Switch Control Pins
  pinMode(MOTORSW_LOCK, INPUT);
  pinMode(MOTORSW_UNLOCK, INPUT);

  // Turn off the motor
  digitalWrite(MOTOR1, LOW); // set pin 2 on L293D low
  digitalWrite(MOTOR2, LOW); // set pin 7 on L293D high
  
  return true;
}

bool unlock(){
  Serial.println(F("[status]: Unlocking door..."));
  // Spin motor CW <-> CCW
  digitalWrite(MOTOR1, LOW);  // set pin 2 on L293D low
  digitalWrite(MOTOR2, HIGH); // set pin 7 on L293D high
 
  //read the unlock button here and then decide on it
  bool  unlockSWState = digitalRead(MOTORSW_UNLOCK);

  while(true){
      unlockSWState = digitalRead(MOTORSW_UNLOCK);
      if (unlockSWState == LOW){
          delay(500);
          break;
      }
  }
  // Turn off the motor
  digitalWrite(MOTOR1, LOW); // set pin 2 on L293D low
  digitalWrite(MOTOR2, LOW); // set pin 7 on L293D high
  

  return true;
}
