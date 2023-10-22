/* ----------- Time related functions ----------
 * License: 2-Clause BSD License
 * Copyright (c) 2023 codingABI
 */

 // Manual input of a time component (Hour, minute,...) 
void inputTimeComponent(time_t &changeTime, SevenSegmentTM1637 *display, byte component) {
  long lastMillis;
  int data;
  int oldData = -1;
  byte monthLengthList[]={31,28,31,30,31,30,31,31,30,31,30,31};
  byte monthLength;
  #define MAXSTRDATALENGTH 4
  char strData[MAXSTRDATALENGTH+1];

  if (component == TIMECOMPONENT_DAY) {
    monthLength =  monthLengthList[month(changeTime)-1];
    if (year(changeTime) % 4 == 0) monthLength++; // Leap year
  }
  SERIALDEBUG.print(F("Set ")); 
  switch (component) {
    case TIMECOMPONENT_MINUTE: SERIALDEBUG.println(F("minute"));break;
    case TIMECOMPONENT_HOUR: SERIALDEBUG.println(F("hour"));break;
    case TIMECOMPONENT_YEAR: SERIALDEBUG.println(F("year"));break;
    case TIMECOMPONENT_MONTH: SERIALDEBUG.println(F("month"));break;
    case TIMECOMPONENT_DAY: SERIALDEBUG.println(F("day"));break; 
    default: SERIALDEBUG.println(component);return;
  }
  
  while (!digitalRead(BUTTON_PIN_UP) || !digitalRead(BUTTON_PIN_DOWN)) {} // Wait for button release
  switch (component) {
    case TIMECOMPONENT_MINUTE: data = minute(changeTime);break;
    case TIMECOMPONENT_HOUR: data = hour(changeTime);break;
    case TIMECOMPONENT_YEAR: data = year(changeTime);break;
    case TIMECOMPONENT_MONTH: data = month(changeTime);break;
    case TIMECOMPONENT_DAY: data = day(changeTime);break;
  }

  lastMillis = millis();
  while (millis()-lastMillis <= BUTTONTIMEOUT_MS) { // Loop until button not pressed more than 10 seconds
    wdt_reset();
    switch (component) {
      case TIMECOMPONENT_MINUTE: snprintf(strData,MAXSTRDATALENGTH+1,"  %02d",data);break;
      case TIMECOMPONENT_HOUR: snprintf(strData,MAXSTRDATALENGTH+1,"%02d  ",data);break;
      case TIMECOMPONENT_YEAR: snprintf(strData,MAXSTRDATALENGTH+1,"%04d",data);break;
      case TIMECOMPONENT_MONTH: snprintf(strData,MAXSTRDATALENGTH+1,"nn%02d",data);break;
      case TIMECOMPONENT_DAY: snprintf(strData,MAXSTRDATALENGTH+1,"d %02d",data);break;
    }
    if ((component == TIMECOMPONENT_MINUTE) || (component == TIMECOMPONENT_HOUR)) display->setColonOn(true);
    display->print(strData);

    if (!digitalRead(BUTTON_PIN_UP)) {
      delay(50);
      if (!digitalRead(BUTTON_PIN_DOWN)) { // Both buttons pressed => finish 
        delay(200);
        lastMillis+=BUTTONTIMEOUT_MS;
      } else {
        data++;
        switch (component) {
          case TIMECOMPONENT_MINUTE: if (data > 59) data=0;break;
          case TIMECOMPONENT_HOUR: if (data > 23) data=0;break;
          case TIMECOMPONENT_YEAR: if (data > 2100) data=2100;break; // time_t should work until 2106
          case TIMECOMPONENT_MONTH: if (data > 12) data=1;break;
          case TIMECOMPONENT_DAY: if (data > monthLength) data=1;break;
        }
        delay(200);
        lastMillis = millis();
      }
    } else {
      if (!digitalRead(BUTTON_PIN_DOWN)) {
        delay(50);
        if (!digitalRead(BUTTON_PIN_UP)) { // Both buttons pressed => Next step
          delay(200);
          lastMillis+=BUTTONTIMEOUT_MS;
        } else {
          data--;
          switch (component) {
            case TIMECOMPONENT_MINUTE: if (data < 0) data=59;break;
            case TIMECOMPONENT_HOUR: if (data < 0) data=23;break;
            case TIMECOMPONENT_YEAR: if (data < 2000) data=2000;break;
            case TIMECOMPONENT_MONTH: if (data < 1) data=12;break;
            case TIMECOMPONENT_DAY: if (data < 1) data=monthLength;break;
          }
          delay(200);
          lastMillis = millis();
        }
      }
    }
    if (data != oldData) {
      switch (component) {
        case TIMECOMPONENT_MINUTE: changeTime = tmConvert_t(year(changeTime),month(changeTime),day(changeTime),hour(changeTime),data,0);break;
        case TIMECOMPONENT_HOUR: changeTime = tmConvert_t(year(changeTime),month(changeTime),day(changeTime),data,minute(changeTime),0);break;
        case TIMECOMPONENT_MONTH: changeTime = tmConvert_t(year(changeTime),data,day(changeTime),hour(changeTime),minute(changeTime),0);break;
        case TIMECOMPONENT_YEAR: changeTime = tmConvert_t(data,month(changeTime),day(changeTime),hour(changeTime),minute(changeTime),0);break;
        case TIMECOMPONENT_DAY: changeTime = tmConvert_t(year(changeTime),month(changeTime),data,hour(changeTime),minute(changeTime),0);break;
      }
      oldData = data;
    }
  }

  wdt_reset();
  // Blink
  display->clear();
  delay(BLINKDELAY_MS);
  display->print(strData);
  delay(BLINKDELAY_MS);
  display->setColonOn(false);
  display->clear();
  delay(BLINKDELAY_MS);
}

// Set time manually, when both buttons are pressed
bool checkSetManualTime(bool force) {
  SevenSegmentTM1637 *display; // 7-segment display
  time_t changeTime;
    
  if (force || ((!digitalRead(BUTTON_PIN_UP) && !digitalRead(BUTTON_PIN_DOWN)))) { // Both button pressed or function forced?
    if (force) 
      SERIALDEBUG.println(F("Forced setting time"));
    else
      SERIALDEBUG.println(F("Set time manually"));
    
    digitalWrite(LED_PIN,LOW); // Disable status led
    changeTime = UTCtoLocalTime(now());

    // Enable Vcc for servo and display
    pinMode(FET_PIN, OUTPUT);
    digitalWrite(FET_PIN,LOW);

    display = new SevenSegmentTM1637(TM1637_PIN_CLK, TM1637_PIN_DIO); // Constructor sets pins to output pin mode
    
    // Init display
    display->begin(); 
    display->setBacklight(TM1637_BACKLIGHT);
    display->clear();

    // Blink
    display->print(F("----"));
    delay(BLINKDELAY_MS);
    display->clear();
    delay(BLINKDELAY_MS);
    display->print(F("----"));
    delay(BLINKDELAY_MS);
    display->clear();
    delay(BLINKDELAY_MS);
    wdt_reset();

    // Set time components for minute, hour, year, month and day
    
    inputTimeComponent(changeTime,display,TIMECOMPONENT_MINUTE);

    inputTimeComponent(changeTime,display,TIMECOMPONENT_HOUR);

    inputTimeComponent(changeTime,display,TIMECOMPONENT_YEAR);

    inputTimeComponent(changeTime,display,TIMECOMPONENT_MONTH);

    inputTimeComponent(changeTime,display,TIMECOMPONENT_DAY);
  
    // Turn off display and set pins as input to reduce power consumption
    display->off();
    delete display;
    digitalWrite(TM1637_PIN_CLK,LOW);
    digitalWrite(TM1637_PIN_DIO,LOW);
    pinMode(TM1637_PIN_CLK,INPUT);
    pinMode(TM1637_PIN_DIO,INPUT); 

    if (year(changeTime) >= 2001) {
      SERIALDEBUG.println(F("Manual time set"));
      setTime(localTimeToUTC(changeTime));      
      if ((g_bootTimeUTC == 0) || (now()<g_bootTimeUTC) ) g_bootTimeUTC = now();
      return true; // Time set
    } else SERIALDEBUG.println(F("Manual time not valid"));
  }
  return false;
}

// Check if its time to do a DCF77 sync and start a sync if need
void checkSetTime(bool force) {
  #define DCFTIMEOUTMINUTES 10
  static bool timeSetDailyDone = false;
  #define MAXSTRDATALENGTH 50
  char strData[MAXSTRDATALENGTH+1];
  time_t DCF77time = 0;
  time_t localTime;
  unsigned long startDCF77 = 0;
  DCF77 DCF = DCF77(DCF77_PIN_T,DCF77_INTERRUPT);
  SevenSegmentTM1637 *display; // 7-segment display

  localTime = UTCtoLocalTime(now());

  if (hour(localTime) != TIMESYNCHOUR) timeSetDailyDone = false; 

  if (checkSetManualTime()) return; // No DCF77 sync needed, when time set manually
  
  if (((hour(localTime) == TIMESYNCHOUR) && !timeSetDailyDone) || force) { // Time to sync or function force?
    startDCF77 = millis();
    // Enable Vcc for DCF77 receiver
    pinMode(DCF77_PIN_VCC,OUTPUT);
    digitalWrite(DCF77_PIN_VCC,HIGH);

    if (force) 
      SERIALDEBUG.println(F("Forced DCF77")); 
    else 
      SERIALDEBUG.println(F("Start DCF77"));
    // Start DCF
    DCF.Start();

    // Show DCF77 signals on led
    pinMode(LED_PIN,OUTPUT);

    while (DCF77time==0) {
      digitalWrite(LED_PIN,digitalRead(DCF77_PIN_T));
      wdt_reset();
  
      DCF77time = DCF.getTime(); // Check if new DCF77 time is available
      if (checkSetManualTime()) {
        DCF77time = UTCtoLocalTime(now()); // Overwrite DCF77 time with manually set time        
      }
            
      if (DCF77time!=0) { // Valid time found
        SERIALDEBUG.println(F("DCF77 done"));
        setTime(localTimeToUTC(DCF77time));
        if ((g_bootTimeUTC == 0) || (now()<g_bootTimeUTC)) g_bootTimeUTC = now();
        timeSetDailyDone = true;
      } else {
        // Timeout after 10 minutes without DCF77 sync
        if ((millis()-startDCF77)/1000/60>=DCFTIMEOUTMINUTES) {
          SERIALDEBUG.println(F("DCF77 timed out"));
          beep();
          DCF.Stop();
          pinMode(DCF77_PIN_VCC,INPUT); // Disable Vcc for DCF77 receiver
          pinMode(LED_PIN,INPUT);

          // Force manual time input
          if (checkSetManualTime(true)){
            timeSetDailyDone = true;
            g_weakTime = true;
            return;
          } else {
            // Could not get time. Go do deep sleep for some minutes to save power and then do a WDT reset to try it again
            SERIALDEBUG.println(F("No valid time!"));
            beep(LONGBEEP);

            SERIALDEBUG.println(F("Deep sleep for 15 min."));
            // Disable ADC
            ADCSRA = 0;  
            // Disable TWI (Two Wire Interface) = I2C
            cli();
            v_sleep = true;
            v_sleepCounter = 0;
            sei();
            byte sleepCounter = 0; 
            // Sleep for 15 minutes (15*60/8.192 = 110)
            #define DEEPSLEEPSBEFOREWDTRESET 110
            while (sleepCounter < DEEPSLEEPSBEFOREWDTRESET) { 
              set_sleep_mode(SLEEP_MODE_PWR_DOWN);
              cli();
              sleep_enable();
              sei();
              sleep_cpu();
              sleep_disable();
              sleepCounter++;
              enableWatchdogTimer();
              wdt_reset();
            }
            SERIALDEBUG.println(F("Stop for WDT reset"));
            while (true); // Wait for watchdog reset          
          }
        }
      }
    }
    DCF.Stop();
    pinMode(DCF77_PIN_VCC,INPUT); // Disable Vcc for DCF77 receiver
    pinMode(LED_PIN,INPUT);

    snprintf(strData,MAXSTRDATALENGTH+1,"Set time %04i/%02i/%02i %02i:%02i",year(),month(),day(),hour(),minute());
    SERIALDEBUG.println(strData);

    // Time set done
    g_weakTime = false;
  }
}

// create time_t from time components
time_t tmConvert_t(int YYYY, byte MM, byte DD, byte hh, byte mm, byte ss)
{
  tmElements_t tmSet;
  tmSet.Year = YYYY - 1970;
  tmSet.Month = MM;
  tmSet.Day = DD;
  tmSet.Hour = hh;
  tmSet.Minute = mm;
  tmSet.Second = ss;
  return makeTime(tmSet); 
}

// Convert Germany local time to UTC
time_t localTimeToUTC(time_t localTime) {
  if (summertime_EU(year(localTime),month(localTime),day(localTime),hour(localTime),1)) {
    return  localTime-7200UL; // Summer time
  } else {
    return  localTime-3600UL; // Winter time
  }
}

// Convert UTC to Germany local time
time_t UTCtoLocalTime(time_t UTC) {
  if (summertime_EU(year(UTC),month(UTC),day(UTC),hour(UTC),0)) {
    return UTC+7200UL; // Summer time
  } else {
    return UTC+3600UL; // Winter time
  }
}
