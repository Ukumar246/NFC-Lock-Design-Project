#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#define PN532_SS   (10)
Adafruit_PN532 nfc(PN532_SS);

#define BLOCK_LIMIT (3)

#if defined(ARDUINO_ARCH_SAMD)
    // for Zero, output on USB Serial console, remove line below if using programming port to program the Zero!
    // also change #define in Adafruit_PN532.cpp library file
    #define Serial SerialUSB
#endif

//CODES: Transfer to include file

//for selecting the first request
 uint8_t selectApdu[] =  { 
                                  0x00, /* CLA */
                                  0xA4, /* INS */
                                  0x04, /* P1  */
                                  0x00, /* P2  */
                                  0x0E, /* Lc - Length of AID  0x05 -> 0x0E*/  
                                  0x32, 0x50, 0x41, 0x59, 0x2E, 0x53, 0x59, 0x53, 0x2E, 0x44, 0x44, 0x46, 0x30, 0x31,   /* AID defined on EMV*/
                                  0x00  /* Le  */
                                };
uint8_t visaApdu[] =  { 
                                  0x00, /* CLA */
                                  0xA4, /* INS */
                                  0x04, /* P1  */
                                  0x00, /* P2  */
                                  0x07, /* Lc - Length of AID  0x05 -> 0x0E*/  
                                  0xA0, 0x00, 0x00, 0x00, 0x03, 0x10, 0x10,   /* AID defined on EMV*/
                                  0x00  /* Le  */
                                };
//header for the third round
uint8_t thirdRoundCode []= { 
                                  0x80, /* CLA */
                                  0xA8, /* INS */
                                  0x00, /* P1  */
                                  0x00, /* P2  */
                         };
 uint8_t terminalTransQualifiers []= {
                                        0x28,
                                        0x00,
                                        0x00,
                                        0x00,
                                      
                                      };

        

uint8_t authorizedAmount []= {
                                  0x00,
                                  0x00,
                                  0x00,
                                  0x00,
                                  0x00,
                                  0x00,                                    
                             };
uint8_t randomNumber []= {
                                  0x00,
                                  0x00,
                                  0x00,
                                  0x00,
                                          
                         };
 uint8_t transactionCurrencyCode []= {
                                      0x09,
                                      0x78,
                                    };
//DEFAULT CODE HEADER STRUCTURE FOR NORMAL CREDIT CARDS
  uint8_t requestCardNumber[] =  { 
                                  0x80, /* CLA */
                                  0xA8, /* INS */
                                  0x00, /* P1  */
                                  0x00, /* P2  */
                                  0x12, /* PDOL lengths added together --- 4+6+4+2 = 16 + 2 = 18 (the plus 2 is for the 0x83 and the 0x00 of Le)---- the country code is irrelevant*/  
                                  0x83, 0x10, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x24,/* AID defined on EMV*/
                                  0x00 /* Le  */
                                };
uint8_t requestApplePayAccount[] =  { 
                                  0x80, /* CLA */
                                  0xA8, /* INS */
                                  0x00, /* P1  */
                                  0x00, /* P2  */
                                  0x37, /* Lc - Length of AID  0x05 -> 0x0E*/  
                                  0x83, 0x35, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00,0x00,0x00,0x00,0x02,0x50,0x00,0x00,0x00,0x00,0x00,0x09,0x78,0x16,0x10,0x05,0x00,0x8F,0x9C,0x20,0xEE,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, /* AID defined on EMV*/
                                  0x00 /* Le  */
                                };

                                        // Setup for PN532 Shield
void setup(void){
  #ifndef ESP8266
    while (!Serial); 
  #endif
  
  // has to be fast to dump the entire memory contents!
  Serial.begin(115200);
  Serial.println("Looking for PN532...");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  // configure board to read RFID tags
  nfc.SAMConfig();
  Serial.println("Waiting for an ISO14443A card..");
}

void loop(void) {
    bool success;
    
    success = nfc.inListPassiveTarget();
    if(success) {
        uint8_t response[255];
        uint8_t responseLength = sizeof(response);

        success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);

        //TODO: the second header has corner case for hte application ID not being same for all the cards.
        if(success) {
           do{
                uint8_t back[255];
                uint8_t length = 255; 

                //tester
                Serial.println("BACK ARRAY CONTENTS AS PASSED IN: ");
               
                success = nfc.inDataExchange(visaApdu, sizeof(visaApdu), back, &length);

                //From back  you can detect the kind of card you are reading example VISA, MASTERCARD---
                if (success){
                   Serial.println("function Print: ");printArray(back,sizeof(back)); 
                   nfc.PrintHexChar(back, length);
                   uint8_t pdolLengths = totalPdolLengths (back, sizeof(back));
                   Serial.print("Total lengths of PDOL RETURNED:  ");Serial.println(pdolLengths);
                   readVisaCardNumber(success,pdolLengths);
                }else{
                  Serial.println("Could not send second header---Please hold card for longer-------"); 
                }
           }while(success);
        }
        else{
            Serial.println("Came out of success while loop---Failed reading data---"); 
        }
        delay(200);
    }
}

void readVisaCardNumber(bool success, uint8_t pdolLengths){
// Print Data
           Serial.println("VISA Card Detected");
//            nfc.PrintHexChar(response, responseLength);
Serial.println("PDOL lengths inside readVisa: "); Serial.print(pdolLengths);
          
           uint8_t creditCardNumber[8];
//           tester for the compare credit card function
           uint8_t testerCardNumber [8];
          
            do {
                 uint8_t userInfo[255];
                 uint8_t length = 255; 

                 if (pdolLengths<=16){
                  //normal credit cards (wallet)
                     success = nfc.inDataExchange(requestCardNumber, sizeof(requestCardNumber), userInfo, &length);
                 }else{
                    //apple pay which requires more information
                    success = nfc.inDataExchange(requestApplePayAccount, sizeof(requestApplePayAccount), userInfo, &length);
                 }
                
                //this reponse has the user information: credit card number etc.
                if (success){
                  Serial.print("3rd Round: "); Serial.println(length);
                  nfc.PrintHexChar(userInfo, length);

                  uint8_t i = 0;
                  bool debugger = false;
                  uint8_t decider =0;
                  for (i=0; i<sizeof(userInfo); i++){
                    uint8_t responseDigit = userInfo[i];
                    //credit card data in track 2. This is in second track between 57 13 (45 20 01 00 43 08 75 45) D1
                    //D1=209
                    if (responseDigit==209 || responseDigit == 210){
                        debugger = true;
//                      Serial.print("Individual Digit Code: "); Serial.println(responseDigit);Serial.println("at the index: ");Serial.println(i);
                      //you;ve found the D1
                      //now loop back 8 sections of the array to get the 8 bytes of the credit card number
                      Serial.println("FOUND : ");Serial.println(responseDigit,HEX);
                      uint8_t j = i;
                      uint8_t k = 0;
//                        Serial.print("You Credit Card Number: ");
                        for (j=j-8, k=0; j<i; j++, k++){
//                           Serial.println(userInfo[j],HEX);
                           //size of credit card cannot be more than 8 bytes
                           if (k<8){
                              //store the decoded credit card number into the credit card array
                              creditCardNumber[k]=userInfo[j];
                              //TODO: only for testing purposes. remove testerCardNumber later
                              testerCardNumber[k] = userInfo[j];
                           }else{
                            Serial.println("Indexing Issue: card k != j");
                           }
                        }
                        
                    }
          
                  }

                  if (debugger == false){
                    Serial.println("This card does not contain the end of track 2 value D1 or D2. Please try another card");
                  }
//                  if true the continue..

                  uint8_t w=0;
                  Serial.println("Credit Card Number Stored in the Array: ");
                  for (w=0; w<sizeof(creditCardNumber);w++){
                    Serial.print(creditCardNumber[w],HEX);
                  }   
                  Serial.print("\n");               
              }
        else {
          Serial.println("Broken connection---please hold the card for longer---SUCCESS returned false"); 
        }
      }
      while(success);
}


//Utility Function for comparing credit cards:
bool compareCardNumber(uint8_t *firstCard, uint8_t firstCardLength, uint8_t *secondCard, uint8_t secondCardLength){
  
      //IMPORTANT: the numbers in the credit card arrays are stored as hex values
      
      if (firstCardLength!=secondCardLength){
        //if size not eaual they are different cards
        return false;
      }
      //assuming the lengths are equal
      uint8_t i = 0;
      for (i=0; i<firstCardLength; i++){
         if (firstCard[i]!=secondCard[i]){
            //if any digit does not match return false
            return false;
         }
      }
  
      //once past the for loop you know that all digits are the same and cards match
      return true;

}


//function to add header
void printArray(uint8_t array[], uint8_t count){
    uint8_t i = 0; 
    for (i=0; i< count; i++){
        Serial.print(array[i],HEX);
    }
}

//function to dynamically calculate the numberof pdol length options
uint8_t totalPdolLengths (uint8_t back[], uint8_t count){
       uint8_t pdolLengths =0;

      Serial.print("2nd Round: "); Serial.println(count);
      //printArray(back);
      uint8_t j = 0;
      for (j=0; j<count;j++){      
         
         //9F=159 38=56
         if (back[j]==159 && back[j+1]==56){
          //found the PDOL start
          //not super relevant
           uint8_t pdolLength = back[j+2];
           Serial.print("found the PDOL section: "); Serial.println(pdolLength,HEX);
          }else if (back[j]==159 && back[j+1]==102){
            //Terminal Transaction Qualifiers
            pdolLengths+=back[j+2];         
          }else if (back[j]==159 && back[j+1]==2){
            //amount authorized
             pdolLengths+=back[j+2]; 
          }else if (back[j]==159 && back[j+1]==55){
             //Unpredictable number
             pdolLengths+=back[j+2];
          }else if (back[j]==95 && back[j+1]==42){
            //TRansaction Currency Code
            pdolLengths+=back[j+2];
          }else if (back[j]==149){
            //TVR
            pdolLengths+=back[j+1];
          }else if (back[j]==154){
            //Transaction Date
            pdolLengths+=back[j+1];
          }else if(back[j]==156){
            //Transaction Type
            pdolLengths+=back[j+1];
          }else if (back[j]==159 && back[j+1]==78){
            //Merchant Name and Location
             pdolLengths+=back[j+2];
          }else if (back[j]==159 && back[j+1]==26){
            //Terminal Country Code
             pdolLengths+=back[j+2];
          }
      }
       return pdolLengths;

  }

uint8_t* constructVisaCardHeader(uint8_t pdolLengths){
      //Magic number 6 includes the 2 bytes for 0x83 and the LE byte 00 + the header 0x80,0xA8,0x00,0x00
      uint8_t totalLength = pdolLengths + 8;
      uint8_t testerHeader[totalLength];
      uint8_t i =0;
      //CONSTRUCT SENDING MESSAGE 3---------------------------------------------------------------------------------------------------------------------------------------
    
    //ADD HEADER:works
      for (i=0; i<4; i++){
        testerHeader[i]=thirdRoundCode[i];
      }
      
    //  ADD 0x83 and actual pdol Length (i should be 4 here)
        Serial.println("i of pdolLengths: ");  Serial.println(i);
      testerHeader[i] = pdolLengths+2;
      i++;
      Serial.println("pdolLenghts +2: ");  Serial.println(pdolLengths+2);
       Serial.println("i of 0x83: ");  Serial.println(i);
      testerHeader[i]= 131;
    
      i++;
       Serial.println("i of lengths without 83: ");  Serial.println(i);
      testerHeader[i]=pdolLengths;
    
      i++;
    // ADD the terminal transaction qualifiers
      uint8_t j = 0;
      Serial.println("i of start of term qualifiers: ");  Serial.println(i);
      testerHeader[i]=pdolLengths;
      for (j=0;j<sizeof(terminalTransQualifiers);j++,i++){
        Serial.print("The terminal transaction quals: "); Serial.print(terminalTransQualifiers[j]);
        testerHeader[i]=terminalTransQualifiers[j];
      }
    
      // it is 11
      Serial.println("i of end of term qualifiers: ");  Serial.println(i);
    //  ADD the authorized amount
    
       for (j=0; j<sizeof(authorizedAmount); j++,i++){
          Serial.print("The authorized amout quals: "); Serial.print(authorizedAmount[j]);
          testerHeader[i]=authorizedAmount[j];
       }
    
     Serial.println("i of end of authorized amount: ");  Serial.println(i);
    
     
       for (j=0; j<sizeof(randomNumber); j++,i++){
          Serial.print("The random number quals: "); Serial.print(randomNumber[j]);
          testerHeader[i]=randomNumber[j];
       }
    
     Serial.println("i of end of randomNumber: ");  Serial.println(i);
    
    // transactionCurrencyCode
    
     for (j=0; j<sizeof(transactionCurrencyCode); j++,i++){
          Serial.print("The transaction currenc code quals: "); Serial.print(transactionCurrencyCode[j]);
          testerHeader[i]=transactionCurrencyCode[j];
       }
    
     Serial.println("i of end of transactionCurrencyCode: ");  Serial.println(i);
    
    // LE 
    
      testerHeader[i] = 0;
    
    //print the complete header to send
      for (i=0; i<sizeof(testerHeader); i++){
        Serial.println(testerHeader[i],HEX);
      }

    return testerHeader;
      
}

