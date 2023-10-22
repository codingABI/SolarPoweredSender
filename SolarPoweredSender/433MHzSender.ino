/* ----------- 433 MHz sender related functions ----------
 * License: 2-Clause BSD License
 * Copyright (c) 2023 codingABI
 */

// Enable sender
void initSender() {
  if (DEBUG) {
    SERIALDEBUG.println(F("Skip 433 MHz sender in debug mode"));
    beep(HIGHSHORTBEEP);
    delay(10);
    return; // Do not use sender in DEBUG mode because used sender pins would conflict with "Serial"
  } 
  if (g_RCSwitch == NULL) {
    beep(HIGHSHORTBEEP);
    // Enable Vcc for sender
    pinMode(RCSWITCH_PIN_VCC,OUTPUT);
    digitalWrite(RCSWITCH_PIN_VCC,HIGH);
    delay(10);  
    // Pin mode for dat pin
    pinMode(RCSWITCH_PIN_DAT,OUTPUT);
    // 433 MHz sender
    g_RCSwitch = new RCSwitch();
    if (g_RCSwitch != NULL) {
      g_RCSwitch->enableTransmit(RCSWITCH_PIN_DAT); 
      g_RCSwitch->setRepeatTransmit(5);
    } else { // Exception, should not happen
      beep(LONGBEEP);
      // Enable red led
      pinMode(LED_PIN,OUTPUT);
      digitalWrite(LED_PIN,HIGH);
      while (true); // Wait for watchdog reset, when RCSwitch could not be created       
    }
  }
}

// Disable sender
void deinitSender() {
  if (DEBUG) {
    return; // Do not use sender in DEBUG mode because used sender pins would conflict with "Serial"
  }
    
  if (g_RCSwitch != NULL) {
    g_RCSwitch->disableTransmit();

    digitalWrite(RCSWITCH_PIN_DAT,LOW);
    pinMode(RCSWITCH_PIN_DAT,INPUT);

    delete g_RCSwitch;
    g_RCSwitch = NULL;

    // Disable Vcc for sender
    digitalWrite(RCSWITCH_PIN_VCC,LOW);
    pinMode(RCSWITCH_PIN_VCC,INPUT);
  }
}

// Send Vcc and runtime
void sendDailyStatus(int Voltage_mv, bool lowBattery) {
  SERIALDEBUG.println(F("Send daily status"));
  initSender();
  if (g_RCSwitch) {
    g_RCSwitch->setProtocol(1);
    g_RCSwitch->send(RCSIGNATURE +
      (((unsigned long) ID & 7) <<24) + 
      ((unsigned long) lowBattery << 23) + 
      (((((unsigned long) Voltage_mv+50)/100) & 63) << 17)+
      (((now() - g_bootTimeUTC) / 86400UL) & 1023), 32);
  }
}

// Send ON/OFF signals to 433 MHz power outlets, if its time to do
void checkOutlets() {
  time_t localTime;

  localTime = UTCtoLocalTime(now());

  /* Bath: renkforce RSL366R from Conrad
   * Device 1 
   * ON: 1381717 1 OFF: 1381716
   */
  if (g_Outlets.getPlanedStatus(OUTLET_BATH,hour(localTime),minute(localTime),dayOfWeek(localTime)) == OutletControl::StartOutlet) { 
    initSender();

    if (g_RCSwitch) {
      SERIALDEBUG.println(F("Send bath ON"));    
      g_RCSwitch->setProtocol(1);
      g_RCSwitch->send(1381717,24);
    }
    g_Outlets.setStatus(OUTLET_BATH,OutletControl::StartOutlet); 
  }
  if (g_Outlets.getPlanedStatus(OUTLET_BATH,hour(localTime),minute(localTime),dayOfWeek(localTime)) == OutletControl::StopOutlet) { 
    initSender();
    if (g_RCSwitch) {
      SERIALDEBUG.println(F("Send bath OFF"));
      g_RCSwitch->setProtocol(1);
      g_RCSwitch->send(1381716,24);
    }
    g_Outlets.setStatus(OUTLET_BATH,OutletControl::StopOutlet); 
  }

/* Cafe machine: Emil Lux 315606 power outlets from OBI
 * Device B:
 * ON: 14729269, 15601397, 15168997, 15390725
 * OFF: 14969669, 15327941, 15680661, 15113429
 */
  if (g_Outlets.getPlanedStatus(OUTLET_CAFEMACHINE,hour(localTime),minute(localTime),dayOfWeek(localTime)) == OutletControl::StartOutlet) { 
    initSender();
    if (g_RCSwitch) {
      SERIALDEBUG.println(F("Send cafe ON"));    
      g_RCSwitch->setProtocol(4,350);
      g_RCSwitch->send(14729269,24);
    }
    g_Outlets.setStatus(OUTLET_CAFEMACHINE,OutletControl::StartOutlet); 
  }
  if (g_Outlets.getPlanedStatus(OUTLET_CAFEMACHINE,hour(localTime),minute(localTime),dayOfWeek(localTime)) == OutletControl::StopOutlet) { 
    initSender();
    if (g_RCSwitch) {
      SERIALDEBUG.println(F("Send cafe OFF"));    
      g_RCSwitch->setProtocol(4,350);
      g_RCSwitch->send(14969669,24);
    }
    g_Outlets.setStatus(OUTLET_CAFEMACHINE,OutletControl::StopOutlet); 
  }

/* Living room: Emil Lux 315606 power outlets from OBI
 * Device A:
 * ON: 15267436, 15448348, 14912860, 15475116
 * OFF: 14766972, 15013772, 14826028, 15577020
 */
  if (g_Outlets.getPlanedStatus(OUTLET_LIVINGROOM,hour(localTime),minute(localTime),dayOfWeek(localTime)) == OutletControl::StartOutlet) { 
    initSender();
    if (g_RCSwitch) {
      SERIALDEBUG.println(F("Send living room ON"));    
      g_RCSwitch->setProtocol(4,350);
      g_RCSwitch->send(15267436,24);
    }
    g_Outlets.setStatus(OUTLET_LIVINGROOM,OutletControl::StartOutlet); 
  }
  if (g_Outlets.getPlanedStatus(OUTLET_LIVINGROOM,hour(localTime),minute(localTime),dayOfWeek(localTime)) == OutletControl::StopOutlet) { 
    initSender();
    if (g_RCSwitch) {
      SERIALDEBUG.println(F("Send living room OFF"));    
      g_RCSwitch->setProtocol(4,350);
      g_RCSwitch->send(14766972,24);
    }
    g_Outlets.setStatus(OUTLET_LIVINGROOM,OutletControl::StopOutlet); 
  }
}
