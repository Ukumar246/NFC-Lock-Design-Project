

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
  0x83, 0x35, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x78, 0x16, 0x10, 0x05, 0x00, 0x8F, 0x9C, 0x20, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* AID defined on EMV*/
  0x00 /* Le  */
};

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
