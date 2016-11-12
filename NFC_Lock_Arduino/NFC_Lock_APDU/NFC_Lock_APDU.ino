
#include "NFC_Lock_APDU.h"
/*
 * Setup the pin for the PN_532
 */
Adafruit_PN532 nfc(PN532_SS);

/*
 * PN_532 set up
 */
 
void setup(void) {
#ifndef ESP8266
  while (!Serial);
#endif

  Serial.begin(115200);
  Serial.println("Looking for PN532...");
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    
    Serial.print("Didn't find PN53x board");
    while (1); // halt
    
  }
  
  //Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX); Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC); Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  Serial.println("Waiting for an ISO14443A card..");
  
//
//  if (!lockHWSetup())
//  {
//     Serial.println("Lock HW Setup Failed!");
//     while(!lockHWSetup());
//  }

  Serial.println("Ready...");
}

void loop(void) {
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
          
        } else {
          
          Serial.println("Could not send second header---Please hold card for longer-------");
          
        }
      
    }
    else {
      
      Serial.println("Could not read the first response from the card----- Hold card again!------");
      delay(200);
      
    }
  
  }
}


//bool lockHWSetup()
//{
//  pinMode(MOTOR1, OUTPUT);
//  pinMode(MOTOR2, OUTPUT);
//
//  // Turn off the motor
//  digitalWrite(MOTOR1, LOW); // set pin 2 on L293D low
//  digitalWrite(MOTOR2, LOW); // set pin 7 on L293D high
//  
//  return true;
//}

//bool unlock(){
//  Serial.println("[status]: Unlocking door...");
//  digitalWrite(MOTOR1, LOW); // set pin 2 on L293D low
//  digitalWrite(MOTOR2, HIGH); // set pin 7 on L293D high
// 
//  delay(500);   //1sec
//
//  // Turn off the motor
//  digitalWrite(MOTOR1, LOW); // set pin 2 on L293D low
//  digitalWrite(MOTOR2, LOW); // set pin 7 on L293D high
//  return true;
//}

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

    //this reponse has the user information: credit card number etc.
    if (success) {
      
      Serial.print("3rd Round (User information): ");
      nfc.PrintHexChar(userInfo, length);

      uint8_t i = 0;
      for (i = 0; i < sizeof(userInfo); i++) {
        uint8_t responseDigit = userInfo[i];
        //credit card data in track 2. This is in second track between 57 13 (45 20 01 00 43 08 75 45) D1
        //D1=209
        if (responseDigit == 209 || responseDigit == 210) {
        
          //Found D1
          //now loop back 8 sections of the array to get the 8 bytes of the credit card number
          Serial.println("Found D1 : "); Serial.println(responseDigit, HEX);
          
          uint8_t j = i;
          uint8_t k = 0;
          //Serial.print("You Credit Card Number: ");
          for (j = j - 8, k = 0; j < i; j++, k++) {
        
            //size of credit card cannot be more than 8 bytes
            if (k < 8) {
              
              //store the decoded credit card number into the credit card array
              creditCardNumber[k] = userInfo[j];
              
            } else {
              
              Serial.println("Indexing Issue: card k != j");
              
            }
          }

        }

     }

      Serial.println("Credit Card Number Stored: ");
      printArray(creditCardNumber,sizeof(creditCardNumber));
      
      //unlock();
      Serial.println("Unlocking Door--- ");
      delay(3000);
      Serial.println("Read card again...");
    }
    else {
      Serial.println("Broken connection--Third and final response was not recieved correctly---Failed to read Credit Card");
    }
 
}


/**
 * Utilities Below: 
 * printArray: Prints array when given the array and the length of the array
 * compareCardNumber: compares whether two cards are the same given their contents and lengths
 */
 

void printArray(uint8_t array[], uint8_t count) {
  uint8_t i = 0;
  for (i = 0; i < count; i++) {
    Serial.print(array[i], HEX);
  }
  Serial.print("\n");
}

bool compareCardNumber(uint8_t *firstCard, uint8_t firstCardLength, uint8_t *secondCard, uint8_t secondCardLength) {

  //IMPORTANT: the numbers in the credit card arrays are stored as hex values

  if (firstCardLength != secondCardLength) {
    //if size not eaual they are different cards
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

  Serial.print("2nd Round: "); Serial.println(count);
  //printArray(back);
  uint8_t j = 0;
  for (j = 0; j < count; j++) {

    //9F=159 38=56
    if (back[j] == 159 && back[j + 1] == 56) {
      //found the PDOL start
      //not super relevant
      uint8_t pdolLength = back[j + 2];
      Serial.print("found the PDOL section: "); Serial.println(pdolLength, HEX);
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


