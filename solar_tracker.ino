#include <Servo.h>

Servo horizontal;  // Horizontal Servo Motor
int servohori = 90;
int servohoriLimitHigh = 175;
int servohoriLimitLow = 5;

Servo vertical;  // Vertical Servo Motor
int servovert = 90;
int servovertLimitHigh = 175;
int servovertLimitLow = 5;

// LDR module connections (using A0 pins)
int ldrTopLeft = A0;      // Top Left LDR
int ldrTopRight = A1;     // Top Right LDR
int ldrBottomRight = A2;  // Bottom Right LDR
int ldrBottomLeft = A3;   // Bottom Left LDR

void setup() {
  horizontal.attach(3);  // Horizontal servo on pin 3
  vertical.attach(13);   // Vertical servo on pin 13
  horizontal.write(servohori);
  vertical.write(servovert);
  delay(2500);
}

void loop() {
  // Read LDR values from A0 pins
  int tl = analogRead(ldrTopLeft);      // Top Left
  int tr = analogRead(ldrTopRight);     // Top Right
  int br = analogRead(ldrBottomRight);  // Bottom Right
  int bl = analogRead(ldrBottomLeft);   // Bottom Left

  int dtime = 10;
  int tol = 90;  // Tolerance value for adjustment

  // Calculate averages
  int avt = (tl + tr) / 2;  // Average value of top sensors
  int avd = (bl + br) / 2;  // Average value of bottom sensors
  int avl = (tl + bl) / 2;  // Average value of left sensors
  int avr = (tr + br) / 2;  // Average value of right sensors

  // Calculate differences
  int dvert = avt - avd;   // Difference between top and bottom
  int dhoriz = avl - avr;  // Difference between left and right

  // Vertical movement adjustment - REVERSED
  if (abs(dvert) > tol) {
    if (avt > avd) {
      // More light on top, move DOWN (reversed)
      servovert = --servovert;
      if (servovert < servovertLimitLow) servovert = servovertLimitLow;
    } else {
      // More light on bottom, move UP (reversed)
      servovert = ++servovert;
      if (servovert > servovertLimitHigh) servovert = servovertLimitHigh;
    }
    vertical.write(servovert);
  }

  // Horizontal movement adjustment - REVERSED
  if (abs(dhoriz) > tol) {
    if (avl > avr) {
      // More light on left, move RIGHT (reversed)
      servohori = ++servohori;
      if (servohori > servohoriLimitHigh) servohori = servohoriLimitHigh;
    } else {
      // More light on right, move LEFT (reversed)
      servohori = --servohori;
      if (servohori < servohoriLimitLow) servohori = servohoriLimitLow;
    }
    horizontal.write(servohori);
  }

  delay(dtime);
}