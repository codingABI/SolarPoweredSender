/* ----------- EEPROM related functions ----------
 * License: 2-Clause BSD License
 * Copyright (c) 2023 codingABI
 */

// Read settings from EEPROM
void readSettingsFromEEPROM() {
  #define MAXSTRDATALENGTH 50
  char strData[MAXSTRDATALENGTH+1];
  int servoParkedDay, servoParkedMonth, servoParkedYear;
  int addr = EEPROM_STARTADDR;
  
  if ((EEPROM.read(addr) == EEPROM_SIGNATURE) && (EEPROM.read(addr+sizeof(byte)) == EEPROM_VERSION) ) {
    SERIALDEBUG.println(F("EEPROM data found:"));
    addr+=2*sizeof(byte);
    // Read day of last servo parking time from EEPROM
    servoParkedYear = 2000+ EEPROM.read(addr);
    addr+=sizeof(byte);
    servoParkedMonth = EEPROM.read(addr);
    addr+=sizeof(byte);
    servoParkedDay = EEPROM.read(addr);
    addr+=sizeof(byte);
    g_servoFeedbackValue0 = (EEPROM.read(addr)<<8) + EEPROM.read(addr+sizeof(byte));
    addr+=2*sizeof(byte);
    g_servoFeedbackValue180 = (EEPROM.read(addr)<<8) + EEPROM.read(addr+sizeof(byte));
    addr+=2*sizeof(byte);

    if ((servoParkedDay >= 1) && (servoParkedDay <= 31) && (servoParkedMonth >= 1) && (servoParkedMonth <= 12) && (servoParkedYear <= 2100)) {
      g_lastServoParkingUTC = tmConvert_t(servoParkedYear,servoParkedMonth,servoParkedDay,0,0,0);  
      snprintf(strData,MAXSTRDATALENGTH+1," Last servo parking UTC %04i/%02i/%02i",year(g_lastServoParkingUTC),month(g_lastServoParkingUTC),day(g_lastServoParkingUTC));
      SERIALDEBUG.println(strData);
    }
    SERIALDEBUG.println(F(" Feedback values"));
    SERIALDEBUG.print(F("  0="));
    SERIALDEBUG.println(g_servoFeedbackValue0);
    SERIALDEBUG.print(F("  180="));
    SERIALDEBUG.println(g_servoFeedbackValue180);
  } else SERIALDEBUG.println(F("No EEPROM data found"));
}

// Store settings in EEPROM
void writeSettingsToEEPROM() { 
  #define MAXSTRDATALENGTH 50
  char strData[MAXSTRDATALENGTH+1];
  int addr = EEPROM_STARTADDR;

  SERIALDEBUG.println(F("Store settings to EEPROM:"));
  snprintf(strData,MAXSTRDATALENGTH+1," Last servo parking UTC %04i/%02i/%02i",year(g_lastServoParkingUTC),month(g_lastServoParkingUTC),day(g_lastServoParkingUTC));
  SERIALDEBUG.println(strData);
  SERIALDEBUG.println(F(" Feedback values"));
  SERIALDEBUG.print(F("  0="));
  SERIALDEBUG.println(g_servoFeedbackValue0);
  SERIALDEBUG.print(F("  180="));
  SERIALDEBUG.println(g_servoFeedbackValue180);

  // Save values in EEPROM
  EEPROM.update(addr,EEPROM_SIGNATURE);
  addr+=sizeof(byte);
  EEPROM.update(addr,EEPROM_VERSION);
  addr+=sizeof(byte);
  EEPROM.update(addr, year(g_lastServoParkingUTC)-2000);
  addr+=sizeof(byte);
  EEPROM.update(addr, month(g_lastServoParkingUTC));
  addr+=sizeof(byte);
  EEPROM.update(addr, day(g_lastServoParkingUTC));
  addr+=sizeof(byte);
  EEPROM.update(addr, (g_servoFeedbackValue0>>8) & 0xff);
  addr+=sizeof(byte);
  EEPROM.update(addr, g_servoFeedbackValue0 & 0xff);
  addr+=sizeof(byte);
  EEPROM.update(addr, (g_servoFeedbackValue180>>8) & 0xff);
  addr+=sizeof(byte);
  EEPROM.update(addr, g_servoFeedbackValue180 & 0xff);
}
