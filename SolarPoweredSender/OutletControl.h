/* --- Class to simplify powering ON/OFF the power outlets by schedule ---
 * License: 2-Clause BSD License
 * Copyright (c) 2023 codingABI
 */
#pragma once

/*
 * Basis is a on/off list, where each list entry consists of 
 * - the number of the power outlet
 * - Hour, minute and day for the power outlet action
 * - the power outlet action: "on" or "off"
 * The day can be a weekday (Monday, Tuesday ... Sunday) or all days of a week
 */
 
class OutletControl {
  public:
    static const byte Sunday=1;
    static const byte Monday=2;
    static const byte Tuesday=4;
    static const byte Wednesday=8;
    static const byte Thursday=16;
    static const byte Friday=32;
    static const byte Saturday=64;
    static const byte AllDays=127;
    static const byte StartOutlet=128;
    static const byte StopOutlet=0;
    static const byte NOP=255;
    
    OutletControl();
    int addTime(byte Outlet, byte Hour, byte Minute, byte Day, byte Action);
    byte setStatus(byte Outlet, byte Status);
    byte getStatus(byte Outlet);
    byte getPlanedStatus(byte Outlet, byte Hour, byte Minute, byte dayOfTheWeek);

  private:
    struct SimpleTime { byte Outlet; byte Hour; byte Minute; byte Day; };
    byte _StartsStops;
    static const byte MaxStartsStops=10;
    static const byte MaxOutlets=3;
    SimpleTime _StartStopTime[MaxStartsStops];
    byte _Status[MaxOutlets];
};

// Constructor for the scheduler
OutletControl::OutletControl() {
  _StartsStops=0; // Clear ON/OFF list
  for (int i;i<MaxOutlets;i++) { _Status[i] = NOP; } // Clear internal ON/OFF states
}
// Add a ON or OFF signal for a power outlet to the scheduler 
int OutletControl::addTime(byte Outlet, byte Hour, byte Minute, byte Day, byte Action) {
  if (_StartsStops >= MaxStartsStops) return (-1); // ON/OFF list full
  _StartStopTime[_StartsStops].Outlet = Outlet;
  _StartStopTime[_StartsStops].Hour = Hour;
  _StartStopTime[_StartsStops].Minute = Minute;
  // Store ON/OFF signal as 8th bit of the weekday
  _StartStopTime[_StartsStops].Day = Day | Action;
  _StartsStops++;
  return(_StartsStops);
}
// Set the current internal ON/OFF state for a power outlet 
byte OutletControl::setStatus(byte Outlet, byte Status) {
  if (Outlet >= MaxOutlets) return NOP; // Returns NOP, when number of outlets exceeds the maximum number
  _Status[Outlet] = Status;
}
// Get the current internal ON/OFF state for a power outlet
byte OutletControl::getStatus(byte Outlet) {
  if (Outlet >= MaxOutlets) return NOP; // Returns NOP, when number of outlets exceeds the maximum number
  return(_Status[Outlet]);
}
// Get the target ON/OFF state of a power outlet for a given time 
byte OutletControl::getPlanedStatus(byte Outlet, byte Hour, byte Minute, byte dayOfTheWeek) {
  byte planedState;
  for (int i=0;i<_StartsStops;i++) {
    // Is given time in the same 5 minute slot of a scheduled ON/OFF signal?
    if ((_StartStopTime[i].Outlet==Outlet) && (_StartStopTime[i].Hour==Hour) && (_StartStopTime[i].Minute/5==Minute/5) && ((1 << (dayOfTheWeek-1)) & _StartStopTime[i].Day)) {
      // Return target action (or NOP, when noting has to be done)
      planedState = _StartStopTime[i].Day & 128;
      if (planedState == _Status[Outlet]) return NOP;
      return planedState; 
    }
  }
  return NOP;
}
