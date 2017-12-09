#include "Sense_ADS131A04.h"
#include <SPI.h>

/* ----------------------- Internal Functions --------------------------------*/
uint32_t parseM(unsigned char BUFFER[3], int SRT) {
  uint32_t STATUS = ((BUFFER[0] << 8) & 0xFF00) | ((BUFFER[1]) & 0x00FF); // Read first 16 bits of buffer

  // Send or receive switch case
  switch (SRT) {
    case 0:
      Serial.print("R: ");
      break;
    case 1:
      Serial.print("S: ");
      break;
  }

  printHex(STATUS, 4);
  Serial.print(" = ");

  // Decode cases and determine returned STATUS messages
  switch (STATUS) {
    case 0x0000:
      Serial.println("NULL");
      break;
    case 0x0011:
      Serial.println("RESET");
      break;
    case 0xFF82:
      Serial.println("RESETTING...");
      break;
    case 0xFF04:
      Serial.println("READY");
      break;
    case 0x0022:
      Serial.println("STANDBY...");
      break;
    case 0x0033:
      Serial.println("WAKING UP...");
      break;
    case 0x0555:
      Serial.println("LOCKED");
      break;
    case 0x0655:
      Serial.println("UNLOCKED");
      break;

    default:
      //If none of these, check R/W REG
      switch ((STATUS & 0xE000) >> 13) {
        case 0b010:
          Serial.println("WREG SENT");
          break;
        case 0b001:
          Serial.println("RREG");
          break;
        default:
          Serial.println("STATUS ERROR");
          break;
      }
      break;
  }
  return (STATUS);
}

void printHex(int num, int precision) {
     char tmp[16];
     char format[128];

     sprintf(format, "0x%%.%dX", precision);

     sprintf(tmp, format, num);
     Serial.print(tmp);
}



/* ----------------------- Main External Functions ---------------------------*/

Sense_ADS131A04::Sense_ADS131A04(){
  union ID_MSB;
  union F_STAT_1;
  union F_STAT_P;
  union F_STAT_N;
  union F_STAT_S;
  union F_ERROR_CNT;
  union F_STAT_M2;
  union F_CONF_A_SYS_CFG;
  union F_CONF_D_SYS_CFG;
  union F_CLK1;
  union F_CLK2;
  union F_ADC_ENA;
  union F_ADC1;
  union F_ADC2;
  union F_ADC3;
  union F_ADC4;
}

void Sense_ADS131A04::config(
  union F_ADC_ENA
){

}

void Sense_ADS131A04::setup(){
  Serial.begin(9600);
  Serial.println("Setting up...");
  SPI.begin();
  //SPI.setClockDivider(SPI_CLOCK_DIV8);

  pinMode(SSP, OUTPUT);
  pinMode(DRDY, INPUT);
  delay(1000); // Allow for startup time
  //adsWriteCommand(UNLOCK, UNLOCK); // Unlock registers for setting WREG if restarting
  adsWriteCommand(RESET, READY); // Ensure POR
  adsWriteCommand(UNLOCK, UNLOCK); // Unlock registers for setting WREG
  adsWriteRegister(0x0B, 0b11101000); // charge pump enable, internal reference enable
  adsWriteRegister(0x0C, 0b00111100); // Set fixed frame size
  adsWriteRegister(0x0D, 0b00000010); // Set internal clock to use OSC
  adsWriteRegister(0x0E, 0b00100000); // Set ICLK divider ratio and OSR
  adsWriteRegister(0x11, 0b00000100);
  adsWriteRegister(0x12, 0b00000100);
  adsWriteRegister(0x13, 0b00000100);
  adsWriteRegister(0x14, 0b00000100);
}

void Sense_ADS131A04::adsWriteCommand(uint32_t COMMAND, uint32_t STATUS) {
  unsigned char SND_BUF[3];
  unsigned char REC_BUF[3];
  uint32_t RETURNED_STATUS = 0x00;

  Serial.println(); // Newline for serial monitor cleanliness

  // Begin SPI transaction using designated settings


  // Rerun transaction if status not received
  while (RETURNED_STATUS != STATUS) {

    // Format command into I/O buffer MSB -> first 16 bits

    SND_BUF[0] = (COMMAND & 0xFF00) >> 8;
    SND_BUF[1] = COMMAND & 0xFF;
    SND_BUF[2] = 0x00;

    REC_BUF[0] = 0x00;
    REC_BUF[1] = 0x00;
    REC_BUF[2] = 0x00;

    parseM(SND_BUF, SENT); // Parse received message

    // Enable ~CS then transfer I/O buffer, get response
    SPI.beginTransaction(SPISettings(SPI_CLOCK_SPEED, MSBFIRST, SPI_MODE1));
    digitalWrite(SSP, LOW);
    SPI.transfer(&SND_BUF, 3);
    digitalWrite(SSP, HIGH);

    digitalWrite(SSP, LOW);
    SPI.transfer(&REC_BUF, 3);
    digitalWrite(SSP, HIGH);

    // Response is given to message on subsequent data frame
    SPI.endTransaction();
    RETURNED_STATUS = parseM(REC_BUF, RECEIVED);
  }

void Sense_ADS131A04::adsWriteRegister(unsigned char REG_ADDRESS, uint32_t REG_DATA){
  Serial.print("\nWriting Register ");
  Serial.print(REG_ADDRESS, HEX);
  Serial.print(" with data ");

  // Ensure register message is in format 0b010a aaaa nnnn nnnn with reg address a* and data n*
  uint32_t FORMATTED_WREG_COMMAND = ((0b010 << 13) | (REG_ADDRESS << 8) | (REG_DATA & 0xFF)) & 0xFFFFFF;
  Serial.print(REG_DATA, BIN);
  uint32_t EXPECTED_STATUS = (0b001 << 13) | (REG_ADDRESS << 8) | (REG_DATA & 0xFF);
  adsWriteCommand(FORMATTED_WREG_COMMAND, EXPECTED_STATUS);
}

void Sense_ADS131A04::adsReadRegister(){
  unsigned char REC_BUF[3] = {0x00, 0x00, 0x00};
  unsigned char SND_BUF[3];
  Serial.print("\nReading Register ");
  Serial.println(REG_ADDRESS, HEX);
  uint32_t FORMATTED_RREG_COMMAND = ((0b001 << 13) | (REG_ADDRESS << 8)) & 0xFFFF00;

  SPI.beginTransaction(SPISettings(SPI_CLOCK_SPEED, MSBFIRST, SPI_MODE1));
  SND_BUF[0] = (FORMATTED_RREG_COMMAND >> 8) & 0xFF;
  SND_BUF[1] = FORMATTED_RREG_COMMAND & 0xFF;
  SND_BUF[2] = 0x00;
  parseM(SND_BUF, SENT);

  // Enable ~CS then transfer I/O buffer, get response

  digitalWrite(SSP, LOW);
  SPI.transfer(&SND_BUF, 3);
  digitalWrite(SSP, HIGH);

  digitalWrite(SSP, LOW);
  SPI.transfer(&REC_BUF, 3);
  digitalWrite(SSP, HIGH);
  SPI.endTransaction();
  parseM(REC_BUF, RECEIVED);

}

void Sense_ADS131A04::adsSample(){
  uint8_t BUF_SIZE = 15;
  unsigned char SND_BUF[BUF_SIZE];
  long CH1, CH2, CH3, CH4;
  float CH1_f, CH2_f, CH3_f, CH4_f;
  uint8_t i;

  for(i = 0; i < BUF_SIZE; i++){
    SND_BUF[i] = 0x00;
  }
  //Serial.println("DRDY");
  // Begin SPI transaction using designated settings
  SPI.beginTransaction(SPISettings(SPI_CLOCK_SPEED, MSBFIRST, SPI_MODE1));
  digitalWrite(SSP, LOW);
  SPI.transfer(&SND_BUF, BUF_SIZE);
  digitalWrite(SSP, HIGH);

  SPI.endTransaction();

  /*for(i = 0; i < BUF_SIZE; i++){
    Serial.print(SND_BUF[i], HEX);
  }*/
  CH1 = (SND_BUF[3] << 16) | (SND_BUF[4] << 8) | SND_BUF[5];
  CH2 = (SND_BUF[6] << 16) | (SND_BUF[7] << 8) | SND_BUF[8];
  CH3 = (SND_BUF[9] << 16) | (SND_BUF[10] << 8) | SND_BUF[11];
  CH4 = (SND_BUF[12] << 16) | (SND_BUF[13] << 8) | SND_BUF[14];

  if (CH1 & 0x00800000){
    CH1 |= 0xFF000000 ;
  }
  if (CH2 & 0x00800000){
    CH2 |= 0xFF000000 ;
  }
  if (CH3 & 0x00800000){
    CH3 |= 0xFF000000 ;
  }
  if (CH4 & 0x00800000){
    CH4 |= 0xFF000000 ;
  }

  CH1_f = (CH1 / 16777216.0) * (4.80000);
  CH2_f = (CH2 / 16777216.0) * (4.80000);
  CH3_f = CH3 * (2* 2.40000 / 16777216);
  CH4_f = CH4 * (2* 2.40000 / 16777216);
}
