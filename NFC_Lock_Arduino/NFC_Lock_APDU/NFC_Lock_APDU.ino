#include <stdint.h>
#include <stdbool.h>
#include "NFC_Lock_APDU.h"
bool programming_mode = true;
uint8_t stored_credit_card_number[8];

/*
 * Setup the pin for the PN_532
 */
Adafruit_PN532 nfc(PN532_SS);

void printPN53xInfo(uint32_t versiondata){
  Serial.print("Found chip PN5"); 
  Serial.println((versiondata >> 24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); 
  Serial.print((versiondata >> 16) & 0xFF, DEC); 
  Serial.print('.'); 
  Serial.println((versiondata >> 8) & 0xFF, DEC);
  return;
}
 
void setup(void) {
  #ifndef ESP8266
    while (!Serial);
  #endif
  
  //Wait for computer
  while(!Serial);

  Serial.begin(115200);
  Serial.println(F("Looking for PN532..."));
  nfc.begin();

//  clearEEPROM();
//  EEPROM.begin(256);

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {    
    Serial.print(F("[SPI ERROR]: Didn't find PN53x board"));
    while (1); // halt
  }

  //Print board info
  printPN53xInfo(versiondata);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  Serial.println(F("Waiting for an NFC card ..."));
 
  if (!lockHWSetup()){
     Serial.println(F("Lock HW Setup Failed!"));
     while(!lockHWSetup());
  }

  Serial.println(F("Ready."));
}

void loop(void) {

  if (programming_mode==true){
      Serial.println(F("Please tap your card to SAVE data."));
  }
  else{
      //Serial.println(F("Please tap your card to UNLOCK door."));
  }

  readCreditCard();
}

/*
 * @Description: readCreditCard deal with sending the intial and second headers
 *               for communicating with the card. The final one is sent via readVisaCardNumber.
 *               This is seperated into a function as is called in the programmable mode 
 *               and the  regular mode for unlocking.
 */

void readCreditCard(){
  bool success;

  success = nfc.inListPassiveTarget();
  if (success) {
    uint8_t response[255];
    uint8_t responseLength = 255;

    success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);

    //TODO: the second header has corner case for hte application ID not being same for all the cards.
    if (success) {
 
        uint8_t back[255];
        uint8_t length = 255;

        success = nfc.inDataExchange(visaApdu, sizeof(visaApdu), back, &length);

        //From back  you can detect the kind of card you are reading example VISA, MASTERCARD---
        if (success) {
          
          uint8_t pdolLengths = totalPdolLengths (back, sizeof(back));
          readVisaCardNumber(success, pdolLengths);
          
        } else {
          
          Serial.println(F("Could not send second header---Please hold card for longer-------"));
          
        }
      
    }
    else {
      
      Serial.println(F("Could not read the first response from the card----- Hold card again!------"));
      delay(200);
      
    }
  
  }
}

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

/*
 * @Description: Can only process VISA cards as it sends the required information over.
 */

void readVisaCardNumber(bool success, uint8_t pdolLengths) {
    uint8_t creditCardNumber[8];
    uint8_t userInfo[255];
    uint8_t length = 255;

    if (pdolLengths <= 16) {
      //normal credit cards (wallet)
      success = nfc.inDataExchange(requestCardNumber, sizeof(requestCardNumber), userInfo, &length);
      
    } else {      
      //apple pay:  which requires more payment options info
      success = nfc.inDataExchange(requestApplePayAccount, sizeof(requestApplePayAccount), userInfo, &length);
    }

    //This reponse has the user information: credit card number etc.
    if (success) {
      Serial.print(F("3rd Round (User information): "));
      nfc.PrintHexChar(userInfo, length);

      uint8_t i = 0;
      for (i = 0; i < sizeof(userInfo); i++) {
        uint8_t responseDigit = userInfo[i];
        
        //credit card data in track 2. This is in second track between 57 13 (45 20 01 00 43 08 75 45) D1=209/D2=210
        if (responseDigit == 209 || responseDigit == 210) {
        
          //Found D1 (End of credit card track): Now loop back 8 sections of the array to get the 8 bytes of the credit card number
          Serial.println(F("Found D1 : ")); Serial.println(responseDigit, HEX);
          
          uint8_t j = i;
          uint8_t k = 0;
          
          for (j = j - 8, k = 0; j < i; j++, k++) {
            //size of credit card cannot be more than 8 bytes
            if (k < 8) {
              //store the decoded credit card number into the credit card array
              creditCardNumber[k] = userInfo[j];
            } else {
              Serial.println(F("[Panic]: Size of credit card number more than 8 bytes>"));
            }
          }
        }
     }

      Serial.println(F("Credit Card Number Stored: "));
      printArray(creditCardNumber,sizeof(creditCardNumber));
      
      /*
      if (programming_mode){

        //save the card number to EEPROM.
        //writeEEPROM(creditCardNumber,sizeof(creditCardNumber));
        memcpy(stored_credit_card_number, creditCardNumber, 8 * sizeof(uint8_t));
        programming_mode=false;
      }else{
          bool matched = compareCardNumber(creditCardNumber ,sizeof(creditCardNumber), stored_credit_card_number, 8);
          if(matched){
            // Unlock Door here
          }
          else{
            Serial.println(F("[ERROR:] Your card does not match!"));
          }
      }
      */
    }
    else {
      Serial.println(F("Broken connection--Third and final response was not recieved correctly---Failed to read Credit Card"));
    }
 
}


/**
 * Utilities Below: 
 * @Description: printArray -  Prints array when given the array and the length of the array
 * @Description: compareCardNumbe -  compares whether two cards are the same given their contents and lengths
 */
 

void printArray(uint8_t array[], uint8_t count) {
  uint8_t i = 0;
  for (i = 0; i < count; i++) {
    Serial.print(array[i], HEX);
  }
  Serial.print(F("\n"));
}

bool compareCardNumber(uint8_t *firstCard, uint8_t firstCardLength, uint8_t *secondCard, uint8_t secondCardLength) {

  //IMPORTANT: the numbers in the credit card arrays are stored as hex values

  if (firstCardLength != secondCardLength) {
    //if somehow size not eaual they are different cards
    return false;
  }
  //assuming the lengths are equal
  uint8_t i = 0;
  for (i = 0; i < firstCardLength; i++) {
    if (firstCard[i] != secondCard[i]) {
      //if any digit does not match return false
      return false;
    }
  }

  Serial.println(F("Comparator --- Your saved Card Number Was: "));
  for (i = 0; i < firstCardLength; ++i) { Serial.print(firstCard[i],HEX); }
  Serial.println(F("Comparator --- Your tapped card number was: "));
  for (i = 0; i < secondCardLength; ++i) { Serial.print(secondCard[i],HEX); }
  
  //once past the for loop you know that all digits are the same and cards match
  return true;
}


/*
 * @Description: Calculates the total length of the Processing Data Options the credit card requires.
 *               The length of the PDOL is used to diffentiate between regular visa cards which require
 *               less options and Apple Pay.
 */

uint8_t totalPdolLengths (uint8_t back[], uint8_t count) {
  uint8_t pdolLengths = 0;

  Serial.print(F("2nd Round: ")); Serial.println(count);
  //printArray(back);
  uint8_t j = 0;
  for (j = 0; j < count; j++) {

    //9F=159 38=56
    if (back[j] == 159 && back[j + 1] == 56) {
      
      //found the PDOL start
      uint8_t pdolLength = back[j + 2];
      Serial.print(F("found the PDOL section: ")); Serial.println(pdolLength, HEX);
      
    } else if (back[j] == 159 && back[j + 1] == 102) {
      
      //Terminal Transaction Qualifiers
      pdolLengths += back[j + 2];
      
    } else if (back[j] == 159 && back[j + 1] == 2) {
      
      //amount authorized
      pdolLengths += back[j + 2];
      
    } else if (back[j] == 159 && back[j + 1] == 55) {
      
      //Unpredictable number
      pdolLengths += back[j + 2];
      
    } else if (back[j] == 95 && back[j + 1] == 42) {
      
      //TRansaction Currency Code
      pdolLengths += back[j + 2];
      
    } else if (back[j] == 149) {
      
      //TVR
      pdolLengths += back[j + 1];
      
    } else if (back[j] == 154) {
      
      //Transaction Date
      pdolLengths += back[j + 1];
      
    } else if (back[j] == 156) {
      
      //Transaction Type
      pdolLengths += back[j + 1];
      
    } else if (back[j] == 159 && back[j + 1] == 78) {
      
      //Merchant Name and Location
      pdolLengths += back[j + 2];
      
    } else if (back[j] == 159 && back[j + 1] == 26) {
      
      //Terminal Country Code
      pdolLengths += back[j + 2];
      
    }
  }
  
  return pdolLengths;
}

/*
 * @Description: Write to EEPROM:
 * 
 */
/*
void writeEEPROM(uint8_t arr[], uint8_t count){
   uint8_t i = 0;
   uint8_t buff[8];
   Serial.println(F("Writing to EEPROM..."));
   for (i = 0; i < count; ++i) { EEPROM.write(i, arr[i]); }

   for (i = 0; i < count; ++i) { buff[i]=EEPROM.read(i); }

   Serial.println(F("Saving your credit card..."));
   Serial.println("Your saved Card Number: ");
   for (i = 0; i < count; ++i) { Serial.print(buff[i],HEX); }

   Serial.println(F("EEPROM Write Finished!"));
}

void clearEEPROM()
{
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  return;
}
*/

/*
 * @Description: Read from EEPROM and then compare the card numbers
 * @Parameter: 
 * 
 */
/*
 void readEEPROMAndCompare(uint8_t arr[] , uint8_t count){
    uint8_t i = 0;
    uint8_t buff[8];
    bool matched = false;
    
    Serial.println(F("Reading from EEPROM..."));
    for (i = 0; i < count; ++i) { buff[i]=EEPROM.read(i); }
    Serial.println(F("Your saved Card Number: "));
    for (i = 0; i < count; ++i) { Serial.print(buff[i],HEX); }

    matched = compareCardNumber(buff,sizeof(buff), arr, count);

    if(matched==true){
      
        unlock();
        Serial.println(F("Unlocking Door... "));
        delay(3000);
        Serial.println(F("Read card again..."));
        
    }else{
        
       Serial.println(F("Your card does not match--- Please try again with another card----"));
        
    }  
 }
*/
