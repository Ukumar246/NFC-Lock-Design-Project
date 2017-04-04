#include "NFC_Lock_master.h"
#if defined(ARDUINO_SAM_DUE)
  //Due specific code
#else
  #error Unsupported hardware
  //#include <EEPROM.h>
#endif

// Uncomment these lines to enable debug output for PN532(SPI) and/or MIFARE related code
#define SETUPDEBUG
#define NFCDEBUG
// If using Native Port on Arduino Zero or Due define as NFCPRINTUSB
#define NFCPRINT Serial
//#define NFCPRINT SerialUSB   // < This is for Native Port
bool programming_mode = true;

/// Setup the pin for the PN_532
Adafruit_PN532 nfc(PN532_SS);

/*
* lifecycle: setup
*/
void setup(void) {
    #ifndef ESP8266
      while (!NFCPRINT);
    #endif

    NFCPRINT.begin(115200);
    NFCPRINT.println("Looking for PN532...");

    nfc.begin();
    //EEPROM.begin(512);

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
        NFCPRINT.print("Didn't find PN53x board");
        while (1); // halt
    }

    #ifdef SETUPDEBUG
        NFCPRINT.print("Found chip PN5"); 
        NFCPRINT.println((versiondata >> 24) & 0xFF, HEX); 
        NFCPRINT.print("Firmware ver. "); 
        NFCPRINT.print((versiondata >> 16) & 0xFF, DEC); NFCPRINT.print('.'); 
        NFCPRINT.println((versiondata >> 8) & 0xFF, DEC);
    #endif

    // configure board to read RFID tags
    nfc.SAMConfig();
    NFCPRINT.println("Waiting for an ISO14443A card..");

    /*
    if (!lockHWSetup()){
        NFCPRINT.println("lock setup failed!");
        while(!lockHWSetup());
    }
    */

    NFCPRINT.println("Ready...");
}

void loop(void) {
    if (programming_mode==true){
        NFCPRINT.println("programming mode");
    }
    else{
        NFCPRINT.println("use mode");
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
    uint8_t responseLength = sizeof(response);

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
        } 
        else {
            #ifdef NFCDEBUG
                NFCPRINT.println("Could not send second header---Please hold card for longer-------");
            #endif
        }
    }
    else {
        #ifdef NFCDEBUG
            NFCPRINT.println("Could not read the first response from the card----- Hold card again!------");
        #endif

        delay(200);
    }  
  }
}

bool lockHWSetup(){
    pinMode(MOTOR1, OUTPUT);
    pinMode(MOTOR2, OUTPUT);

    // Turn off the motor
    digitalWrite(MOTOR1, LOW); // set pin 2 on L293D low
    digitalWrite(MOTOR2, LOW); // set pin 7 on L293D high

    return true;
}

bool unlock(){
    NFCPRINT.println("[status]: Unlocking door...");
    digitalWrite(MOTOR1, LOW); // set pin 2 on L293D low
    digitalWrite(MOTOR2, HIGH); // set pin 7 on L293D high

    delay(500);   //1sec

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
    } 
    else {
      //apple pay:  which requires more payment options info
      success = nfc.inDataExchange(requestApplePayAccount, sizeof(requestApplePayAccount), userInfo, &length);
    }


    //This reponse has the user information: credit card number etc.
    if (success == true){
        #ifdef NFCDEBUG
            NFCPRINT.print("3rd Round (User information): ");
            nfc.PrintHexChar(userInfo, length);
        #endif

        uint8_t i = 0;
        for (i = 0; i < sizeof(userInfo); i++) {
            uint8_t responseDigit = userInfo[i];
        
            //credit card data in track 2. This is in second track between 57 13 (45 20 01 00 43 08 75 45) D1=209/D2=210
            if (responseDigit == 209 || responseDigit == 210) {
                //Found D1 (End of credit card track): Now loop back 8 sections of the array to get the 8 bytes of the credit card number
                #ifdef NFCDEBUG
                    NFCPRINT.println("Found D1 : "); 
                    Serial.println(responseDigit, HEX);
                #endif
              
                uint8_t j = i;
                uint8_t k = 0;
                //NFCPRINT.print("You Credit Card Number: ");
                for (j = j - 8, k = 0; j < i; j++, k++) {
                    //size of credit card cannot be more than 8 bytes
                    if (k < 8) {
                        //store the decoded credit card number into the credit card array
                        creditCardNumber[k] = userInfo[j];
                    } 
                    else {
                        #ifdef NFCDEBUG
                            NFCPRINT.println("[Panic]: Size of credit card number more than 8 bytes>");
                        #endif
                    }
                }
            }
        }

        NFCPRINT.println("Credit Card Number Stored: ");
        printArray(creditCardNumber,sizeof(creditCardNumber));

        if (programming_mode)
        {
            //save the card number to EEPROM.
            //writeEEPROM(creditCardNumber,sizeof(creditCardNumber));
            programming_mode=false;
        }
        else{
          // readEEPROMAndCompare(creditCardNumber,sizeof(creditCardNumber));
        }
    
        return;
    }
    
    #ifdef NFCDEBUG
        NFCPRINT.println("Broken connection--Third and final response was not recieved correctly---Failed to read Credit Card");
    #endif
}


/**
 * Utilities Below: 
 * @Description: printArray -  Prints array when given the array and the length of the array
 * @Description: compareCardNumbe -  compares whether two cards are the same given their contents and lengths
 */
 

void printArray(uint8_t array[], uint8_t count) {
  uint8_t i = 0;
  for (i = 0; i < count; i++) {
    NFCPRINT.print(array[i], HEX);
  }
  NFCPRINT.print("\n");
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

  #ifdef NFCDEBUG
    NFCPRINT.println("your saved card number Was: ");
  #endif
  
  for (i = 0; i < firstCardLength; ++i){
    NFCPRINT.print(firstCard[i],HEX); 
  }
  
  #ifdef NFCDEBUG
  NFCPRINT.println("your tapped card number was: ");
  #endif 

  for (i = 0; i < secondCardLength; ++i) { 
    NFCPRINT.print(secondCard[i],HEX); 
  }
  
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

  #ifdef NFCDEBUG
    NFCPRINT.print("2nd Round: "); NFCPRINT.println(count);
    //printArray(back);
  #endif

  uint8_t j = 0;
  for (j = 0; j < count; j++) {

    //9F=159 38=56
    if (back[j] == 159 && back[j + 1] == 56) {
      
      //found the PDOL start
      uint8_t pdolLength = back[j + 2];

      #ifdef NFCDEBUG
        NFCPRINT.print("found the PDOL section: "); NFCPRINT.println(pdolLength, HEX);
      #endif
      
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

/// EEPROM Functions live here

/*
 * @Description: Write to EEPROM:
 * 
 */
 /*
void writeEEPROM(uint8_t arr[], uint8_t count){
   uint8_t i = 0;
   uint8_t buff[8];
   NFCPRINT.println("Writing to EEPROM...");
   for (i = 0; i < count; ++i) { EEPROM.write(i, arr[i]); }

   for (i = 0; i < count; ++i) { buff[i]=EEPROM.read(i); }

   NFCPRINT.println("Saving your credit card...");
   NFCPRINT.println("Your saved Card Number: ");
   for (i = 0; i < count; ++i) { NFCPRINT.print(buff[i],HEX); }
   
  
}

// * @Description: Read from EEPROM and then compare the card numbers
// * @Parameter: 
// * 
// *
 void readEEPROMAndCompare(uint8_t arr[] , uint8_t count){
    uint8_t i = 0;
    uint8_t buff[8];
    bool matched = false;
    
    NFCPRINT.println("Reading from EEPROM...");
    for (i = 0; i < count; ++i) { buff[i]=EEPROM.read(i); }
    NFCPRINT.println("Your saved Card Number: ");
    for (i = 0; i < count; ++i) { NFCPRINT.print(buff[i],HEX); }

    matched = compareCardNumber(buff,sizeof(buff), arr, count);

    if(matched==true){
      
        //unlock();
        NFCPRINT.println("Unlocking Door... ");
        delay(3000);
        NFCPRINT.println("Read card again...");
        
    }else{
        
       NFCPRINT.println("Your card does not match--- Please try again with another card----");
        
    }
 }
 */
