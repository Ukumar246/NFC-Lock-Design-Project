#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <DueFlashStorage.h> 

// Global Vars
#define PN532_SS   (10)
#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield
Adafruit_PN532 nfc(PN532_SS);
DueFlashStorage dueFlashStorage;

#define flashAddress (5)
#define greenLedPin (22)
#define redLedPin (23)
#define motorPin (24)
#define motorDuration (700)

// We store credit card data block here 
struct CreditCard {
  uint8_t uid[7];
  char* cardNumber;
  char* expiry;
  char* cardHolderName;
};

CreditCard primaryCard;
uint8_t masterUid[7];

void setup(void) {
  #ifndef ESP8266
    while (!Serial); // for Leonardo/Micro/Zero
  #endif

  // HW Pins setup 
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(motorPin, OUTPUT);
  digitalWrite(motorPin, LOW);
  resetLEDs();
  
  // Serial Setup   
  Serial.begin(115200);
  Serial.println("Hello!");

  // NFC Setup
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  // NFC Shield Version
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0xFF);
  
  // configure board to read RFID tags
  nfc.SAMConfig();

  // Put Board into reset mode
  resetMode();
}

void loop(void) {
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  
  if (success) {
    printCard(uid, uidLength);

    bool match = !(strcmp((char *) uid, (char *)  masterUid));
    if (match){
        Serial.println("True!");    

       // Lock Open Logic:
       unLock(uid, uidLength);
    }
    else{
      Serial.println("False!");
    }
    
  // Wait 1 second before continuing
  delay(1000);
  }
  else
  {
    // PN532 probably timed out waiting for a card
    Serial.println("Timed out waiting for a card");
  }
}

// Print a card based on following @param characteristics
void printCard(uint8_t *uid, uint8_t uidLength)
{
    Serial.println("\n\nFound a card!");
    //Serial.print("UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i=0; i < uidLength; i++) 
    {
      Serial.print(uid[i], HEX); 
    }
    Serial.println("");
}

/// Unlock the lock using the following uuid & length
bool unLock(uint8_t *uid, uint8_t uidLength)
{
    Serial.println("Unlocking door...");
    // Light up green led for 1 sec
    digitalWrite(greenLedPin, HIGH);  
    digitalWrite(redLedPin, LOW);     
    delay(2500);   
    digitalWrite(motorPin, HIGH);     
    delay(motorDuration);                   // Motor spin length
    digitalWrite(motorPin, LOW);

    // Lock the doors now
    lock(uid, uidLength);
    return true;
}

/// Unlock the lock using the following uuid & length
bool lock(uint8_t *uid, uint8_t uidLength)
{
    Serial.println("Locking door...");
    // Light up red led forever
    digitalWrite(greenLedPin, LOW);  
    digitalWrite(redLedPin, HIGH); 
    return true;       
}

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



