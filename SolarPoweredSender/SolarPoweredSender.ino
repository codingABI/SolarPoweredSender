/*
 * Project: SolarPoweredSender (Ding7)
 * Description:
 * A solar powered ATmega328P based DIY device to control 433 MHz ASK power outlets
 * - The power outlets can be switch on or off by a scheduler.
 * - The device gets the time via a DCF77 receiver
 * - The device is powered by a battery buffered solar panel.
 * - The solar panel will rotated to the current sun position (one axis)
 * 
 * License: 2-Clause BSD License
 * Copyright (c) 2023 codingABI
 * For details see: License.txt
 * 
 * created by codingABI https://github.com/codingABI/SolarPoweredSender
 * 
 * External code:
 * I use external code in this project in form of libraries and one small 
 * code piece called summertime_EU, but does not provide these external sources. 
 * If you want to compile my project, you should be able to download the 
 * needed libraries 
 * - Dusk2Dawn (by DM Kishi)
 * - DCF77 (by Thijs Elenbaas) 
 * - Time (by Michael Margolis/Paul Stoffregen)
 * - SevenSegmentTM1637 (by Bram Harmsen) 
 * - RCSwitch (by sui77,fingolfin) 
 * with the Arduino IDE Library Manager. 
 * Only the library SolarPosition (by KenWillmott) needs to downloaded 
 * directly from https://github.com/KenWillmott/SolarPosition). 
 * For details to get the small code piece for summertime_EU "European Daylight Savings 
 * Time calculation by "jurs" for German Arduino Forum" see externalCode.ino. 
 * 
 * Hardware:
 * - Microcontroller ATmega328P (with 16 MHz crystal, Board manager: "Arduino UNO" )  
 * - One red status led
 * - Three push buttons (one only as a physical reset button)
 * - Passive buzzer
 * - DCF77 Time signal receive module DAEV6180B1COB
 * - 433MHz sender STX882 (a FS1000A would also works)
 * - TM1637 7-segment display
 * - MG90S servo (The servo can rotate the solar panel up to 180 degrees)
 * - MOSFET IRF9530NPBF to power on and off the servo and 7-segment display
 * - 18650 Battery (3.7V, 3500mAh) with TP4056 loader and MT3608-Boost Converter to get 5V
 * - HT7333-A voltage regulator to get 3.3V for the DCF77 receiver module
 * - Solar panel 6V 4.5W
 * - 433MHz power outlets (like Emil Lux 315606 from OBI, renkforce RSL366R from Conrad, Elro AB440S from OBI...)
 * 
 * Device behavior
 * - When the device is powered on, a DCF77 time sync is started and the red led is enabled. After 10 minutes without getting a valid time, the user is prompted to input the time. If the user does not set a valid time, the device will sleep for 15 minutes, does a reset after deep sleep and the DCF77 time sync will start again. 
 * - After getting the first valid time the device runs in normal operational mode:
 *   - Every 4 minutes the device checks whether it is time to power ON or OFF a power outlet and if this is true then, the device sends a 433MHz signal to the outlet
 *   - Between sunrise and sunset the solar panel will be rotated to the sun
 *   - When the environment is too dark (e.g. without sun) or battery is low the solar panel will not be rotated
 *   - Once per day after sunset the solar panel will be rotated in middle (=90°) position (="parking position")  
 *   - Once per day at 04:00 the device time will be synced by a DCF77 module
 *   - The device time during the day is not very accurate and can drift several minutes per day, but should be accurate enough for the power outlets (A bigger static drift could be reduced by PERIODICADJUSTTIME_S). The daily DCF77 time sync will clear the drift once per day. 
 *   - When battery is low the device will not send 433 MHz signals to the power outlets
 *   - Once per day at 20:00 the battery voltage and runtime will be sent to my receiver project https://github.com/codingABI/SenderReceiver
 * 
 * Power consumption:
 * - After getting a valid time the device is always 4 minutes in deep sleep, wakes up and stays active typically for about ~0.5 seconds (and longer when parking, showing status values, DCF77 sync, manual setting the time...) and goes again to deep sleep 
 * - In deep sleep: ~3 mW (The MT3608-Boost converter consumes ~2mW idle power)
 * - Active time: up to 1.5W, when the servo is rotating (At begin of the servo rotation for a short time more) => C4 with 470uF helps
 * - Device is still working from February 2022 until now (September 2023) only with solar power and I get no direct sun light from May to August at my device position
 * 
 * How to enable display and show current time and other status information?
 * - Press the "+" or "-" button
 * 
 * How to set the time manually?
 * - Press "+" and "-" buttons at the same time
 * - Now you will be prompted to set minute, hour, year, month and day.
 * - You can change the displayed value with the "+" and "-" buttons.
 * - By pressing "+" and "-" buttons at the same time you set the current value and continue to the next time component (minute -> hour -> year -> month -> day). When you do nothing the current value will be set automatically after 10 seconds. 
 * 
 * Red status led:
 * - One long blink every 16 seconds       = Last DCF77 sync was not successful => weak time
 * - One short blink every 8 seconds       = Low battery (every 16 seconds, if last DCF77 sync was not successful)
 * - Double short blink every 8 seconds    = Environment light not high enough to rotate solar panel (every 16 seconds, if last DCF77 sync was not successful) 
 * - Continuous light for 8 seconds        = Pending WDT device reset
 * - After device startup the red led should be enabled for about 
 *   5-60 seconds and blink afterwards once per second for some 
 *   minutes. Otherwise the DCF77 signal could not be received and 
 *   the DCF77 sync will timeout.
 * 
 * Buzzer-Codes
 * - 1xStandard beep   = Timeout for DCF77 sync or user input 
 * - 1xLaser beep      = Begin of normal device start/power on
 * - 1xLong beep       = Error (If critical, the device will be reset)
 * - 1xHigh short beep = 433 MHz signal was sent
 * - 1xShort beep      = A button was pressed to show current status values
 * 
 * Comments to the TP4056 module:
 * - Red led: Charging
 * - Blue and red led: Solar power too low for charging
 * - Blue led: Battery is fully charged
 * 
 * History:
 * 08.06.2021, Initial version with LDRs for servo orientation
 * 29.06.2021, Add RTC module and calculate servo orientation
 * 01.02.2022, Add 433 MHz sender
 * 03.09.2023, Replace RTC with DCF77 module
 * 26.09.2023, Add support for Emil Lux 315606 power outlets
 */

#include <DCF77.h>
#include <TimeLib.h>
#include <Dusk2Dawn.h>
#include <avr/sleep.h> 
#include <avr/wdt.h>
#include <SevenSegmentTM1637.h>
#include <EEPROM.h>
#include <RCSwitch.h> // Without setting "const unsigned int RCSwitch::nSeparationLimit = 1500;" in RCSwitch.cpp the signal for  my Emil Lux 315606 power outlets are not or wrongly detected as "32bit Protocol: 2". 
#include <SolarPosition.h> // Download from https://github.com/KenWillmott/SolarPosition
#include "secrets.h"
#include "OutletControl.h"

#define DEBUG false // Set true to enable Serial.print and disables the 433MHz sender for debugging
#define SERIALDEBUG if (DEBUG) Serial
#define RCSIGNATURE 0b00111000000000000000000000000000UL // Signature for signals to my receiver device https://github.com/codingABI/SenderReceiver (only the first 5 bits are the signature)
#define ID 4
/* Signal (32-bit):
 * 5 Bit: Signature
 * 3 Bit: ID
 * 1 Bit: Low battery
 * 6 Bit: Vbat (0-63)
 * 7 Bit: free
 * 10 Bit: Runtime in days
*/

// Definitions
#define SERVOPULSETIME_US 20000 // Servo pulse period
#define LDRTHRESHOLD 600 // LDR threshold, which must fall below before rotating the solar panel (higher LDR values => darker environment)
#define VOLTAGELOWTHRESHOLD_MV 3000 // Battery voltage threshold before rotating the solar panel

// EEPROM signature and start address to store last parking time and servo feedback calibration
#define EEPROM_SIGNATURE 7 // First byte at startaddress in EEPROM
#define EEPROM_VERSION 1 // Second byte at startaddress in EEPROM
#define EEPROM_STARTADDR 0 // Startaddress in EEPROM

#define TIMESYNCHOUR 4 // Local time hour of day where time will be synced by the DCF77 receiver (in local time) 
#define DAILYSTATUSHOUR 20 // Local time hour of day where Vcc and runtime will be sent to receiver 
#define PERIODICADJUSTTIME_S 0 // Set only, if needed. Would adjust time every 4 minutes by PERIODICADJUSTTIME_S seconds. Values between 0-15s seems to be OK. 1s would adjusts the time 6min/day clockwise (WDT based on the internal 128kHz RC oscillator, but like AVR says "The 128kHz oscillator is a very low power clock source, and is not designed for high accuracy"). The accuracy seems to be related to Vcc, temperature and chip. 
#define BLINKDELAY_MS 200 // Delay between 7-segment display blinks

// Time components
#define TIMECOMPONENT_MINUTE 1
#define TIMECOMPONENT_HOUR 2
#define TIMECOMPONENT_YEAR 3
#define TIMECOMPONENT_MONTH 4
#define TIMECOMPONENT_DAY 5

#define BUTTONTIMEOUT_MS 10000 // Timeout for manual time component inputs

// Pin definitions
#define SERVO_PIN_PWM 3 // Servo pwm data
#define SERVO_PIN_FEEDBACK A4 // Servo analog feedback input
#define BUTTON_PIN_UP 5 // + button
#define BUTTON_PIN_DOWN 4 // - button
#define LED_PIN 13 // Red led 
#define FET_PIN A2 // Pin to control Vcc for servo and 7-segment display
#define LDR_PIN_ANALOG A5 // LDR analog input
#define LDR_PIN_VCC A1 // LDR Vcc output
#define BATTERYVOLTAGE_PIN A3 // Battery analog input
#define BUZZER_PIN 8 // Buzzer/beeper
/* 433 MHz sender 
 * The decision to use pin 0 and 1 was made because the perfboard
 * was originally made without the 433MHz sender and these two pins were
 * free and reachable, when the sender was needed.
 * 
 * If I would make a new perfboard, I would 
 * - use pin 9 for RCSWITCH_PIN_DAT 
 * - connect Vcc of the sender to Vcc of the 7-segment display or servo
 * => This would make pin 0 and 1 free for "Serial.print"
 */
#define RCSWITCH_PIN_DAT 0 // 433Mhz sender data
#define RCSWITCH_PIN_VCC 1 // 433Mhz sender Vcc
/* DCF77 receiver
 * The decision to use pin 11 was made because the perfboard
 * was originally made without the DCF77 receiver and this pin was
 * free and reachable, when the DCF77 receiver was needed.
 * 
 * If I would make a new perfboard, I would 
 * - use pin 10 for DCF77_PIN_VCC 
 * => This would free pin 11 which is in use while flashing the device
 */
#define DCF77_PIN_VCC 11 // Vcc pin for DCF77 (The output of this pin will be converted to 3.3V by a voltage regulator for the DCF77 receiver)
#define DCF77_PIN_T 2 // DCF77 pin "T Time pulse output" (must be an interrupt capable pin)
#define DCF77_INTERRUPT 0 // Interrupt number associated with pin DCF77_PIN_T
// 7-segment display
#define TM1637_PIN_CLK 7
#define TM1637_PIN_DIO 6
#define TM1637_BACKLIGHT 80

// My 433 MHz power outlets
#define OUTLET_BATH 0
#define OUTLET_CAFEMACHINE 1
#define OUTLET_LIVINGROOM 2

// -------- Global variables --------
volatile byte v_sleepCounter = 0; // Count for deep sleep cycles
volatile bool v_sleep = false; // True, if device is in deep sleep 
volatile bool v_displayRequested = false; // True, when a button was pressed
time_t g_bootTimeUTC=0; // First valid time to get runtime
time_t g_lastServoParkingUTC = 0; // Day of last servo parking (Will be stored/read in/from EEPROM). Parking is once per day
bool g_weakTime = false; // True, if last DCF77 sync timed out, but previous time was valid
int g_currentServoDegree = -180; // Current servo orientation
RCSwitch *g_RCSwitch = NULL; // 433 MHz sender object
OutletControl g_Outlets; // Time scheduler for 433 MHz power outlets
// Analog values for the serve feedback line at 0 and 180 degree
#define DEFAULTSERVOFEEDBACKVALUE0 857
#define DEFAULTSERVOFEEDBACKVALUE180 82
int g_servoFeedbackValue0 = DEFAULTSERVOFEEDBACKVALUE0;
int g_servoFeedbackValue180 = DEFAULTSERVOFEEDBACKVALUE180;

// Beep types for the buzzer
enum beepTypes { DEFAULTBEEP, SHORTBEEP, LONGBEEP, HIGHSHORTBEEP, LASER };
/* Servo rotation types
 * - SLOWROTATE: 6 pulses per degree (6x20ms = 120ms per degree) 
 * - VERYSLOWROTATE: 11 pulses per degree (11x20ms = 220ms per degree)
 */
enum rotateTypes { SLOWROTATE, VERYSLOWROTATE };

// -------- function prototypes --------
void beep(byte=DEFAULTBEEP);
void checkSetTime(bool=false);
bool checkSetManualTime(bool=false);
void servoSlowRotate(int, byte=SLOWROTATE );

// Show status values on the display (and do a servo calibration if button pressed)
void showDisplay(int Voltage_mv, int LDRValue, int targetServoDegree) {
  #define DISPLAYDURATION_MS 1000 // Duration displaying values
  SevenSegmentTM1637 *display; // 7-segment display
  #define MAXSTRDATALENGTH 29
  char strData[MAXSTRDATALENGTH+1];

  SERIALDEBUG.println(F("Status values:"));
  // Enable Vcc for display (and servo) 
  pinMode(FET_PIN, OUTPUT);
  digitalWrite(FET_PIN,LOW);

  int measuredServoDegree = getServoDegree();
  int currentServoDegree = g_currentServoDegree;
  
  // Init display
  display = new SevenSegmentTM1637(TM1637_PIN_CLK, TM1637_PIN_DIO);
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

  // Time
  time_t localTime = UTCtoLocalTime(now());
  display->clear();
  snprintf(strData,MAXSTRDATALENGTH+1,"%02d%02d",hour(localTime),minute(localTime));
  display->setColonOn(true);
  display->print(strData);
  snprintf(strData,MAXSTRDATALENGTH+1,"Local time %04i/%02i/%02i %02i:%02i",year(localTime),month(localTime),day(localTime),hour(localTime),minute(localTime));
  SERIALDEBUG.println(strData);
  delay(DISPLAYDURATION_MS);
  display->setColonOn(false);
  wdt_reset();

  // Battery voltage
  display->clear();
  delay(BLINKDELAY_MS);
  snprintf(strData,MAXSTRDATALENGTH+1,"%04d",Voltage_mv);
  display->print(strData);
  snprintf(strData,MAXSTRDATALENGTH+1,"Vbat %d",Voltage_mv);
  SERIALDEBUG.println(strData);
  delay(DISPLAYDURATION_MS);
  wdt_reset();

  // LDR value
  display->clear();
  delay(BLINKDELAY_MS);
  snprintf(strData,MAXSTRDATALENGTH+1,"%4d",LDRValue);
  display->print(strData);
  snprintf(strData,MAXSTRDATALENGTH+1,"LDR %i",LDRValue);
  SERIALDEBUG.println(strData);
  delay(DISPLAYDURATION_MS);
  wdt_reset();

  // Runtime in days
  display->clear();
  delay(BLINKDELAY_MS);
  unsigned long runtime = (now() - g_bootTimeUTC) / 86400UL;
  if (runtime <= 999) {
    snprintf(strData,MAXSTRDATALENGTH+1,"%03lud", runtime);
  } else {
    snprintf(strData,MAXSTRDATALENGTH+1,"%04lu", runtime);    
  }
  display->print(strData);
  snprintf(strData,MAXSTRDATALENGTH+1,"Runtime %lu days",runtime);
  SERIALDEBUG.println(strData);
  delay(DISPLAYDURATION_MS);
  wdt_reset();

  // Target orientation for solar panel (=azimuth-90 degree)
  display->clear();
  delay(BLINKDELAY_MS);
  snprintf(strData,MAXSTRDATALENGTH+1,"%3i",targetServoDegree);
  display->print(strData);
  display->printRaw(B01100011,3); // Degree sign
  snprintf(strData,MAXSTRDATALENGTH+1,"Target servo %d",targetServoDegree);
  SERIALDEBUG.println(strData);
  delay(DISPLAYDURATION_MS);
  wdt_reset();

  // Measured orientation for solar panel
  display->clear();
  delay(BLINKDELAY_MS);
  snprintf(strData,MAXSTRDATALENGTH+1,"%3i",measuredServoDegree);
  display->print(strData);
  display->printRaw(B01101011,3); // Degree sign
  snprintf(strData,MAXSTRDATALENGTH+1,"Measured servo %d",measuredServoDegree);
  SERIALDEBUG.println(strData);
  delay(DISPLAYDURATION_MS);
  wdt_reset();

  if (currentServoDegree >= 0) {
    // Orientation for solar panel
    display->clear();
    delay(BLINKDELAY_MS);
    snprintf(strData,MAXSTRDATALENGTH+1,"%3do",currentServoDegree);
    display->print(strData);
    snprintf(strData,MAXSTRDATALENGTH+1,"Servo orientation %d",currentServoDegree);
    SERIALDEBUG.println(strData);
    delay(DISPLAYDURATION_MS);
    wdt_reset();
  }

  if (!digitalRead(BUTTON_PIN_UP) || !digitalRead(BUTTON_PIN_DOWN)) { // button "+" or "-" pressed?
    // Calibration rotation to get servo feedback values for 0 and 180 degrees 

    display->clear();

    SERIALDEBUG.println(F("Analog value:"));

    // Feedback line at 0 degree
    servoSlowRotate(0);
    int degree0 = getServoFeedbackAvg();
    snprintf(strData,MAXSTRDATALENGTH+1,"%4d",degree0);
    display->print(strData);
    snprintf(strData,MAXSTRDATALENGTH+1," 0=%d",degree0);
    SERIALDEBUG.println(strData);
    delay(DISPLAYDURATION_MS);
    wdt_reset();
  
    // Feedback line at 180 degree
    servoSlowRotate(180);
    int degree180 = getServoFeedbackAvg();
    display->clear();
    delay(BLINKDELAY_MS);
    snprintf(strData,MAXSTRDATALENGTH+1,"%4d",degree180);
    display->print(strData);
    snprintf(strData,MAXSTRDATALENGTH+1," 180=%d",degree180);
    SERIALDEBUG.println(strData);
    delay(DISPLAYDURATION_MS);
    wdt_reset();

    // Rotate to 90 degree 
    servoSlowRotate(90);

    // Safe feedback values in EEPROM
    g_servoFeedbackValue0 = degree0;
    g_servoFeedbackValue180 = degree180;

    // Save calibration values in EEPROM
    writeSettingsToEEPROM();
  }
  
  // Turn off display and set pins as input to reduce power consumption
  display->off();
  delete display;
  digitalWrite(TM1637_PIN_CLK,LOW);
  digitalWrite(TM1637_PIN_DIO,LOW);
  pinMode(TM1637_PIN_CLK,INPUT);
  pinMode(TM1637_PIN_DIO,INPUT); 

  cli();
  v_displayRequested = false;
  sei();
}

void setup() {
  enableWatchdogTimer(); // Watchdog timer (Start at the begin of setup to prevent a boot loop after after a WDT reset)

  setSyncProvider(NULL); // We will use no time sync provider
  
  SERIALDEBUG.begin(115200);
  SERIALDEBUG.println(F("-- Startup device --"));
  SERIALDEBUG.print(F("Compile time "));
  SERIALDEBUG.println(__DATE__ " " __TIME__);

  beep(LASER); // Startup sound

  analogReference(INTERNAL); // 1.1V reference voltage (we measure the battery voltage with a voltage divider => 1.1V is OK)
  // Initial time
  if (!DEBUG) {
    setTime(tmConvert_t(2000,1,1,0,0,0));
  } else {
    setTime(tmConvert_t(2023,10,17,17,56,0));    
  }

  // Read settings (Last parking time and servo calibration) from EEPROM
  readSettingsFromEEPROM();

  // 433 MHz power outlet schedule  
  g_Outlets.addTime(OUTLET_BATH, 05,00,OutletControl::AllDays,OutletControl::StartOutlet);
  g_Outlets.addTime(OUTLET_BATH, 10,00,OutletControl::AllDays,OutletControl::StopOutlet);
  g_Outlets.addTime(OUTLET_BATH, 20,15,OutletControl::AllDays,OutletControl::StartOutlet);
  g_Outlets.addTime(OUTLET_BATH, 23,00,OutletControl::AllDays,OutletControl::StopOutlet);

  g_Outlets.addTime(OUTLET_LIVINGROOM, 00,15,OutletControl::AllDays,OutletControl::StopOutlet);
  g_Outlets.addTime(OUTLET_LIVINGROOM, 06,30,OutletControl::AllDays,OutletControl::StartOutlet);

  g_Outlets.addTime(OUTLET_CAFEMACHINE, 18,30,OutletControl::AllDays,OutletControl::StopOutlet);
  g_Outlets.addTime(OUTLET_CAFEMACHINE, 07,45,OutletControl::Monday|OutletControl::Tuesday|OutletControl::Wednesday|OutletControl::Thursday|OutletControl::Friday,OutletControl::StartOutlet);

  // Pin mode for buttons
  pinMode(BUTTON_PIN_UP,INPUT_PULLUP);
  pinMode(BUTTON_PIN_DOWN,INPUT_PULLUP);
  
  // Pin change interrupts for buttons
  pciSetup(BUTTON_PIN_UP);
  pciSetup(BUTTON_PIN_DOWN);

  // Get first time from DCF77 oder user input 
  if (!DEBUG) checkSetTime(true);
}

void loop() {
  byte backupADCSRA;
  int LDRValue;
  int Voltage_mv;
  #define MAXSTRDATALENGTH 27
  char strData[MAXSTRDATALENGTH+1];
  static bool dailyStatusSent = false;

  float azimuth;
  time_t sunsetUTC, sunriseUTC;
  int targetServoDegree = -1; // Target servo orientation
  byte sleepCounter;
  static byte adjustCounter = 0;
  byte lowBattery = false;
  
  // Check, if it is time to do a DCF77 sync or 
  // the user pressed both buttons to change the time manually
  checkSetTime();

  // Get environment light by LDR
  pinMode(LDR_PIN_VCC,OUTPUT);
  digitalWrite(LDR_PIN_VCC,HIGH);
  delay(BLINKDELAY_MS);
  LDRValue = analogRead(LDR_PIN_ANALOG); 
  if (DEBUG) LDRValue = LDRTHRESHOLD -1;

  SERIALDEBUG.print(F("LDR "));
  SERIALDEBUG.println(LDRValue);
  digitalWrite(LDR_PIN_VCC,LOW);
  pinMode(LDR_PIN_VCC,INPUT);
  
  wdt_reset();
  
  // Get battery voltage 
  Voltage_mv = map(analogRead(BATTERYVOLTAGE_PIN),0,1024,0,6100); 
  if (DEBUG) Voltage_mv=3700;
  SERIALDEBUG.print(F("Vbat "));
  SERIALDEBUG.println(Voltage_mv);

  if (Voltage_mv < VOLTAGELOWTHRESHOLD_MV) {
    lowBattery = true;
    SERIALDEBUG.println(F("Low battery!"));
  }

  // Get calculated sun azimuth
  SolarPosition sunPosition(MYLAT,MYLON);
  azimuth = sunPosition.getSolarAzimuth(now());
  // Get calculated sunset and sunrise
  Dusk2Dawn sunRiseSet(MYLAT,MYLON,0);
  int sunriseMinutes = sunRiseSet.sunrise(year(now()), month(now()), day(now()), false);
  int sunsetMinutes = sunRiseSet.sunset(year(now()), month(now()), day(now()), false);
  sunsetUTC = tmConvert_t(year(now()), month(now()), day(now()), (byte)(sunsetMinutes/60), (byte) (sunsetMinutes%60),0);
  sunriseUTC = tmConvert_t(year(now()), month(now()), day(now()), (byte)(sunriseMinutes/60), (byte) (sunriseMinutes%60),0);
  snprintf(strData,MAXSTRDATALENGTH+1,"UTC %04i/%02i/%02i %02i:%02i",year(),month(),day(),hour(),minute());
  SERIALDEBUG.println(strData);
  time_t localTime = UTCtoLocalTime(now());
  snprintf(strData,MAXSTRDATALENGTH+1,"Local time %04i/%02i/%02i %02i:%02i",year(localTime),month(localTime),day(localTime),hour(localTime),minute(localTime));
  SERIALDEBUG.println(strData);
  SERIALDEBUG.print(F("Azimuth "));
  SERIALDEBUG.println(azimuth);
  snprintf(strData,MAXSTRDATALENGTH+1,"UTC Sunset %02i:%02i",hour(sunsetUTC),minute(sunsetUTC));
  SERIALDEBUG.println(strData);
  snprintf(strData,MAXSTRDATALENGTH+1,"UTC Sunrise %02i:%02i",hour(sunriseUTC),minute(sunriseUTC));
  SERIALDEBUG.println(strData);

  // Calculate solar panel rotation to match sun position
  targetServoDegree = azimuth - 90;
  if (targetServoDegree < 0) targetServoDegree = 0;
  if (targetServoDegree > 180) targetServoDegree = 180;
  SERIALDEBUG.print(F("Target servo orientation "));
  SERIALDEBUG.println(targetServoDegree);

  if (v_displayRequested) { // Display status if requested by button pressed 
    showDisplay(Voltage_mv, LDRValue, targetServoDegree); // Show status values on display
  }

  // Rotate solar panel to sun by servo
  if ((LDRValue < LDRTHRESHOLD) && !lowBattery && (now() > sunriseUTC) && (now() < sunsetUTC)) { // Only if light and battery voltage is high enough and it is daytime (Does not work beyond polar circle)
    SERIALDEBUG.println(F("Rotate servo"));
    // Enable Vcc for servo and display
    pinMode(FET_PIN, OUTPUT);
    digitalWrite(FET_PIN,LOW);

    // Rotate solar panel by servo
    servoSlowRotate(targetServoDegree,VERYSLOWROTATE);
  }

  // Check for 433 MHz outet signals
  if (!lowBattery) checkOutlets();

  // Once per day at 20:00 (local time) send battery status to the "receiver"
  if ((hour(UTCtoLocalTime(now()))==DAILYSTATUSHOUR) && !dailyStatusSent) {
    sendDailyStatus(Voltage_mv,lowBattery);
    dailyStatusSent = true;
  }
  if (hour(UTCtoLocalTime(now()))!=DAILYSTATUSHOUR) { // Reset daily status
    SERIALDEBUG.println(F("Reset daily status"));    
    dailyStatusSent = false; 
  }
  deinitSender();

  // Parking servo
  if (!lowBattery && (now() >= sunsetUTC)) { // Only if battery voltage is high enough and it is daytime (Does not work beyond polar circle)
    // Rotated servo to parking position today?
    if ((year(g_lastServoParkingUTC) != year()) || (month(g_lastServoParkingUTC) != month()) || (day(g_lastServoParkingUTC) != day())) {
      SERIALDEBUG.println(F("Park servo"));
      // Enable Vcc for servo and display
      pinMode(FET_PIN, OUTPUT);
      digitalWrite(FET_PIN,LOW);
 
      // Rotate servo to parking position (90°)
      targetServoDegree = 90;
      servoSlowRotate(targetServoDegree,SLOWROTATE);

      // Store current day in EEPROM
      g_lastServoParkingUTC = now();
      writeSettingsToEEPROM();
    }
  }

  wdt_reset();

  SERIALDEBUG.flush();
  
  // Disable Vcc for servo and display
  digitalWrite(FET_PIN,HIGH);
  pinMode(FET_PIN, INPUT);
  
  // Backup ADC
  backupADCSRA = ADCSRA;
  // Disable ADC
  ADCSRA = 0;  
  // Disable TWI (Two Wire Interface) = I2C
  TWCR = 0;

  cli();
  v_sleep = true;
  v_sleepCounter = 0;
  sleepCounter = 0;
  sei();
  // Earth rotates one degree in 4 minutes => We can sleep 4 minutes (240 seconds)
  #define MAXSLEEPCYCLES 29 // 240 seconds / 8.192s
  while ((sleepCounter < MAXSLEEPCYCLES) && v_sleep) { 
    // millis does not work in deep sleep => Adjust time roughly after sleep
    // WDT sleep time is 8.192s, but the oscillator is not very accurate
    adjustTime(8); // Adjust time by 8 seconds every WDT sleep
    // Every 5th WDT sleep one additional second to get near to 
    // 8.192 seconds avg. The delta of 0.008s will lead to time drift of -85s/day,
    // but the 128kHz RC oszillator is already not very accurate
    if ((adjustCounter%5)==0) {
      adjustTime(1); 
      adjustCounter = 0;
    }
    // LED status
    if (g_weakTime && ((sleepCounter%2)==0)) { // Blink led one long time, when last DCF77 sync did timed out
      pinMode(LED_PIN,OUTPUT);
      digitalWrite(LED_PIN,HIGH);
      delay(500);
      digitalWrite(LED_PIN,LOW);
      pinMode(LED_PIN,INPUT);
      delay(500);
    } else {
      if ((now() > sunriseUTC) && (now() < sunsetUTC)) { // At day time
        if (Voltage_mv < VOLTAGELOWTHRESHOLD_MV) { // Blink led one time, when battery voltage is too low
          pinMode(LED_PIN,OUTPUT);
          digitalWrite(LED_PIN,HIGH);
          delay(100);
          digitalWrite(LED_PIN,LOW);
          pinMode(LED_PIN,INPUT);            
          delay(100);
        } else {
          if (LDRValue >= LDRTHRESHOLD) { // Two short led blinks, when environmental light is too low
            pinMode(LED_PIN,OUTPUT);
            digitalWrite(LED_PIN,HIGH);
            delay(50);
            digitalWrite(LED_PIN,LOW);
            delay(100);
            digitalWrite(LED_PIN,HIGH);
            delay(50);
            digitalWrite(LED_PIN,LOW);
            pinMode(LED_PIN,INPUT);            
            delay(50);
          }
        }
      }
    }

    wdt_reset();

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    cli();
    sleep_enable();
    adjustCounter++;
    sei();

    sleep_cpu();
    sleep_disable(); 

    enableWatchdogTimer();

    cli();
    sleepCounter = v_sleepCounter;
    sei();
  }
  if (PERIODICADJUSTTIME_S >0) adjustTime(round((float) PERIODICADJUSTTIME_S*sleepCounter/MAXSLEEPCYCLES)); // Reduce time shift (proportional to the sleep cycles, because a wakeup by push buttons would shortcut the cycles) 

  cli();
  v_sleep = false;
  sei();

  if (v_displayRequested) beep(SHORTBEEP); // Beep, when display is enabled by button press

  // Restore ADC
  ADCSRA = backupADCSRA;
}
