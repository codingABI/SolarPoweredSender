/* ----------- Some utility functions ----------
 * License: 2-Clause BSD License
 * Copyright (c) 2023 codingABI
 */

// Enable WatchdogTimer
void enableWatchdogTimer() {
  cli();
  // Set bit 3+4 (WDE+WDCE bits) 
  // From Atmel datasheet: "...Within the next four clock cycles, write the WDE and 
  // watchdog prescaler bits (WDP) as desired, but with the WDCE bit cleared. 
  // This must be done in one operation..."
  WDTCSR = WDTCSR | B00011000;
  // Set Watchdog-Timer duration to 8.192 seconds
  WDTCSR = B00100001;
  // Enable Watchdog interrupt by WDIE bit and enable device reset via 1 in WDE bit.
  // From Atmel datasheet: "...The third mode, Interrupt and system reset mode, combines the other two modes by first giving an interrupt and then switch to system reset mode. This mode will for instance allow a safe shutdown by saving critical parameters before a system reset..." 
  WDTCSR = WDTCSR | B01001000;
  sei();
}

// Interrupt service routine for the watchdog timer
ISR(WDT_vect) {
  if (v_sleep) { 
    v_sleepCounter ++; // Count sleep cycles
  } else { // Alert WDT reset!
    // Enable red led
    pinMode(LED_PIN,OUTPUT);
    digitalWrite(LED_PIN,HIGH);
    while (true); // 8 seconds without wdt_reset() will reset the device
  }
}

// Enable pin change interrupt
void pciSetup(byte pin) {
    *digitalPinToPCMSK(pin) |= bit (digitalPinToPCMSKbit(pin));  // enable pin
    PCIFR  |= bit (digitalPinToPCICRbit(pin)); // clear any outstanding interrupt
    PCICR  |= bit (digitalPinToPCICRbit(pin)); // enable interrupt for the group
}

// handle pin change interrupt for D0 to D7 here
ISR (PCINT2_vect) { 
  wdt_reset();
  if (v_sleep) {
    v_sleep = false;
    v_displayRequested = true;
  }
}

// Buzzer
void beep(byte type) {
  pinMode(BUZZER_PIN, OUTPUT);
  switch(type) {
    case DEFAULTBEEP: { // 500 Hz for 200ms
      for (int i=0;i < 100;i++) {
        digitalWrite(BUZZER_PIN,HIGH);
        delay(1);
        digitalWrite(BUZZER_PIN,LOW);
        delay(1);        
      }
      break;
    }
    case SHORTBEEP: { // 1 kHz for 100ms
      for (int i=0;i < 100;i++) {
        digitalWrite(BUZZER_PIN,HIGH);
        delayMicroseconds(500);
        digitalWrite(BUZZER_PIN,LOW);
        delayMicroseconds(500);        
      }
      break;
    }
    case LONGBEEP: { // 250 Hz for 400ms
      for (int i=0;i < 100;i++) {
        digitalWrite(BUZZER_PIN,HIGH);
        delay(2);
        digitalWrite(BUZZER_PIN,LOW);
        delay(2);        
      }
      break;
    }
    case HIGHSHORTBEEP: { // 5 kHz for 100ms
      for (int i=0;i < 500;i++) {
        digitalWrite(BUZZER_PIN,HIGH);
        delayMicroseconds(100);
        digitalWrite(BUZZER_PIN,LOW);
        delayMicroseconds(100);        
      }
      break;
    }
    case LASER: { // Laser like sound
      int i = 5000; // Start frequency in Hz (goes down to 300 Hz)
      int j = 150; // Start duration in microseconds (goes up to 5000 microseconds)      
      while (i>300) {
        i -=50;
        j +=50;
        for (int k=0;k < j/(1000000/i);k++) {
          digitalWrite(BUZZER_PIN,HIGH);
          delayMicroseconds(500000/i);
          digitalWrite(BUZZER_PIN,LOW);
          delayMicroseconds(500000/i);        
        }
        delayMicroseconds(1000);  
      }      
      break;
    }
    default: {
      SERIALDEBUG.print(F("Wrong beep type "));    
      SERIALDEBUG.println(type);
    }
  }
  pinMode(BUZZER_PIN, INPUT);
  delay(50);
  wdt_reset();
}
