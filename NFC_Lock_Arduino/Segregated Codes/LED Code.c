//LED Codes

/// Accept master key (rest mode)
/// In this mode we flash leds to show we are accepting master key:
void resetMode()
{
    // First try to find master key:
    boolean success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
    // ISO14443A type - 'uid' will be populated with the UID
    Serial.println("Waiting for master key...");
    
    // LED Programmming:
    // Both leds are ON
    digitalWrite(greenLedPin, HIGH); 
    digitalWrite(redLedPin, HIGH);  
       
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
    if (success) {
      Serial.println("Setting master card: ");
      Serial.print("Master Key Value: ");
      for (uint8_t i=0; i < uidLength; i++) 
      {
        Serial.print(uid[i], HEX); 
      }
      Serial.println("");
    
      // Copy our master key - global var
      memcpy(masterUid, &uid, sizeof sizeof(uid));

      // LED Programmming:
      // Alternate flashing led 
      delay(200);
      digitalWrite(greenLedPin, LOW); 
      digitalWrite(redLedPin, LOW);  
      delay(200);
      digitalWrite(redLedPin, HIGH);  
      digitalWrite(greenLedPin, LOW);
    } 
}

/// Resets LED's (Default: red is on , green is off)
void resetLEDs()
{
    // Red led is on
    digitalWrite(greenLedPin, LOW);  
    digitalWrite(redLedPin, HIGH);   
}
