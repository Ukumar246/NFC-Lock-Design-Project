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
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  // configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println("Waiting for an ISO14443A card..");
}

void loop(void) {
    bool success;
    success = nfc.inListPassiveTarget();
    if(success) {
        Serial.println("Found something!");
        uint8_t selectApdu[] =  { 
                                  0x00, /* CLA */
                                  0xA4, /* INS */
                                  0x04, /* P1  */
                                  0x00, /* P2  */
                                  0x0E, /* Lc - Length of AID  0x05 -> 0x0E*/  
                                  0x32, 0x50, 0x41, 0x59, 0x2E, 0x53, 0x59, 0x53, 0x2E, 0x44, 0x44, 0x46, 0x30, 0x31,   /* AID defined on EMV*/
                                  0x00  /* Le  */
                                };

        uint8_t response[255];
        uint8_t responseLength = sizeof(response);

        success = nfc.inDataExchange(selectApdu, sizeof(selectApdu), response, &responseLength);
        
        if(success) {
            // Print Data
            Serial.print("responseLength: "); Serial.println(responseLength);
            nfc.PrintHexChar(response, responseLength);
              
            /* Send Second Headers 
            do {
        uint8_t apdu[] = "Hello from Arduino";
        uint8_t back[32];
        uint8_t length = 32; 

        success = nfc.inDataExchange(apdu, sizeof(apdu), back, &length);
        
          if(success) {
            Serial.print("responseLength: "); Serial.println(length);
            nfc.PrintHexChar(back, length);
          }
          else {
           Serial.println("Broken connection?"); 
            }
        }
          while(success);
        */
        }
        else{
            Serial.println("Failed reading data"); 
        }
        delay(200);
    }
}
