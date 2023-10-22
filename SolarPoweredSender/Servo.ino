/* ----------- Servo rotation related functions (= solar panel rotation) ----------
 * License: 2-Clause BSD License
 * Copyright (c) 2023 codingABI
 */

// Calculate pulse high time for degree
unsigned int getPulseHighTime(int degree) {
  // With (180-degree) * 11 + 350; my MG90S would rotate 200 instead of 180 degree
  return (180-degree)*11 + 500;
}

// Rotate servo slowly to the target orientation
void servoSlowRotate(int targetServoDegree, byte speed) {
  int meassuredServoDegree, delta_us;

  switch(speed) {
    case VERYSLOWROTATE: delta_us=1; break; // 11 pulses per degree (11x20ms = 220ms/degree)
    case SLOWROTATE: delta_us=2; break; // 6 pulses per degree (6x20ms = 120ms/degree)
    default: return; // unknown speed (less then 6 pulses per degree skips some rotations)
  }
  
  // Enable servo pin
  pinMode(SERVO_PIN_PWM,OUTPUT);

  meassuredServoDegree = getServoDegree();
  // When internal servo orientation differs to much from the feedback line => Use value from feedback line
  if (abs(g_currentServoDegree - meassuredServoDegree) > 10) {
    g_currentServoDegree = meassuredServoDegree;  
  }

  if (getPulseHighTime(targetServoDegree) < getPulseHighTime(g_currentServoDegree)) delta_us *= -1;

  unsigned int i=getPulseHighTime(g_currentServoDegree); // Start position
  while (i != getPulseHighTime(targetServoDegree)) { // Loop until target position is reached
    i+=delta_us;
    if (((delta_us < 0) && (i<getPulseHighTime(targetServoDegree))) ||
      ((delta_us > 0) && (i>getPulseHighTime(targetServoDegree)))) i = getPulseHighTime(targetServoDegree);
    servoPulse(i);
    wdt_reset();
  }
  // Disable servo pin
  pinMode(SERVO_PIN_PWM,INPUT);

  // Update internal servo orientation value
  g_currentServoDegree = targetServoDegree;
}

// Send servo pulse
void servoPulse(unsigned int pulseHighTime_us) {
  digitalWrite(SERVO_PIN_PWM,HIGH);
  delayMicroseconds(pulseHighTime_us); 
  digitalWrite(SERVO_PIN_PWM,LOW);
  // According to https://www.arduino.cc/reference/en/language/functions/time/delaymicroseconds/ delayMicroseconds can not handle values greater than 16383 
  #define MAXDELAYUS 16383
  unsigned long restDelay_us = SERVOPULSETIME_US-pulseHighTime_us;
  while (restDelay_us > MAXDELAYUS) {
    delayMicroseconds(MAXDELAYUS);
    restDelay_us -= MAXDELAYUS;
  }
  delayMicroseconds(restDelay_us);
}

// Get avg feedback line
int getServoFeedbackAvg() {
  #define LOOPS 3 // Number of measurements to build avg
  unsigned long sum=0;
  delay(50); // Wait for more stable values 
  for (int i=0;i<LOOPS;i++) { // Build avg with short delay
    sum+=analogRead(SERVO_PIN_FEEDBACK);
    delay(20);
  }
  return(sum/LOOPS);  
}

// Get servo orientation by using the feedback line
int getServoDegree() {
  int degree;
  // Servo voltage on feedback line: 1.5V = 90 degree, 0.26V = 0 degree, 2.77V = 180 degree
  // To get the values for the map function you can use CALIBRATIONROTATION
  degree = map(getServoFeedbackAvg(),g_servoFeedbackValue180,g_servoFeedbackValue0,180,0);
  if (degree > 180) degree=180;
  if (degree < 0) degree=0;
  return(degree);
}
