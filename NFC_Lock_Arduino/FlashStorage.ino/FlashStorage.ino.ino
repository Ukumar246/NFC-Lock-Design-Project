/* This example will write a struct to memory which is a very convinient way of storing configuration parameters.
 Try resetting the Arduino Due or unplug the power to it. The values will stay stored. */

#include <DueFlashStorage.h>
DueFlashStorage dueFlashStorage;

// The struct of the configuration.
struct Configuration {
  int32_t bigInteger;
  char* message;
};

// initialize one struct
Configuration configuration;

void setup() {
  Serial.begin(115200);
  delay(500);

  /* Flash is erased every time new code is uploaded. Write the default configuration to flash if first time */
  // running for the first time?
  uint8_t codeRunningForTheFirstTime = dueFlashStorage.read(0); // flash bytes will be 255 at first run
  Serial.print("codeRunningForTheFirstTime: ");
  if (codeRunningForTheFirstTime) {
    Serial.println("yes");
    /* OK first time running, set defaults */
    configuration.bigInteger = 242424; // my lucky number
    configuration.message = "First Time";

    // write configuration struct to flash at adress 4
    byte b2[sizeof(Configuration)]; // create byte array to store the struct
    memcpy(b2, &configuration, sizeof(Configuration)); // copy the struct to the byte array
    dueFlashStorage.write(4, b2, sizeof(Configuration)); // write byte array to flash

    // write 0 to address 0 to indicate that it is not the first time running anymore
    dueFlashStorage.write(0, 0); 
  }
  else {
    Serial.println("no");
  }
}

void loop() {
  Serial.println("Reading...");
  
  /* read configuration struct from flash */
  byte* b = dueFlashStorage.readAddress(4); // byte array which is read from flash at adress 4
  Configuration configurationFromFlash; // create a temporary struct
  memcpy(&configurationFromFlash, b, sizeof(Configuration)); // copy byte array to temporary struct

  // print the content
  Serial.print(" My Integer:");
  Serial.print(configurationFromFlash.bigInteger);

  Serial.print(" Message:");
  Serial.print(configurationFromFlash.message);

  // Spacing
  Serial.println();
  Serial.println();

  /* change some values in the struct and write them back */
  
  // change the message
  String message = configurationFromFlash.message;
  if (configurationFromFlash.message == "First Time"){
      configurationFromFlash.message = "Hello world!";
      
      // write configuration struct to flash at adress 4
      byte b2[sizeof(Configuration)]; // create byte array to store the struct
      memcpy(b2, &configurationFromFlash, sizeof(Configuration)); // copy the struct to the byte array
      dueFlashStorage.write(4, b2, sizeof(Configuration)); // write byte array to flash
  }

  delay(1000);
}








