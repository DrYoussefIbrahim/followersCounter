#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>
#include <SoftwareSerial.h>
#include "LowPower.h"
#include <EEPROM.h>

#define ZERO_SENSE A0
#define DIGIT_SENSE A1

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(5, 8, NEO_GRB + NEO_KHZ800);
SoftwareSerial esp(3, 4);
int brake_values[5] = {1240, 1235, 1995, 1200, 1260};
int speed_values[5] = {500, 500, 1900, 500, 500};
boolean spinning[5] = {0, 0, 0, 0, 0};
boolean lit[5] = {1, 1, 1, 1, 1};
byte servo[5] = {0, 2, 4, 6, 15};
int sense_values[5] = {100, 313, 521, 722, 900};
int values[5] = {0, 0, 0, 0, 0};
long current_num = 1;
byte R = 1;
byte G = 1;
byte B = 3;

void setup() {
  // put your setup code here, to run once:
  pwm.begin();
  pwm.setPWMFreq(200);
  pixels.begin();
  pinMode(ZERO_SENSE, INPUT);
  pinMode(DIGIT_SENSE, INPUT);
  pinMode(2, INPUT);
  pinMode(A3, OUTPUT);
  digitalWrite(A3, HIGH);
  Serial.begin(115200);
  esp.begin(9600);
  stop_all();
  for (byte i = 0; i < 5; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 0)); // off
  }
  pixels.show();
  display_value(0);
  R = EEPROM.read(0);
  G = EEPROM.read(1);
  B = EEPROM.read(2);
}

void loop() {
  attachInterrupt(0, nop, FALLING);
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
  detachInterrupt(0);
  while (!digitalRead(2)) {
    if (esp.available()) {
      delay(1000);
      long input = esp.parseInt();
      Serial.println(input);
      if (input > 99999) {
        set_light_color(input);
      } else {
        display_value(input);
      }
      while (esp.available()) {
        esp.read();
      }
    }
  }
  //  if (Serial.available()) {
  //    delay(1000);
  //    long input = Serial.parseInt();
  //    Serial.println(input);
  //    if (input > 99999) {
  //      set_light_color(input);
  //    } else if (input) {
  //      display_value(input);
  //    }
  //  }
}

void nop() {}

void stop_all() {
  for (byte i = 0; i < 5; i++) {
    if (spinning[i]) {
      if (i == 4) {
        while (analogRead(DIGIT_SENSE) < sense_values[i]);
      }
      pwm.setPWM(servo[i], 0, brake_values[i]);
    }
  }
  delay(20);
  for (byte i = 0; i < 5; i++) {
    if (spinning[4] && i == 4) {
      delay(200);
    }
    pwm.setPWM(servo[i], 0, 0);
    spinning[i] = 0;
  }
}

int analog_grab(boolean request) {
  int buf = 0;
  if (request) {
    for (byte i; i < 10 ; i++) {
      buf += analogRead(DIGIT_SENSE);
    }
  } else {
    for (byte i; i < 10 ; i++) {
      buf += analogRead(ZERO_SENSE);
    }
  }
  Serial.println(buf / 10);
  return buf / 10;
}

void index_digits(boolean request, byte upTo) {
  if (upTo) {
    int state = 0;
    if (request) {
      state = analogRead(DIGIT_SENSE);
    } else {
      state = analogRead(ZERO_SENSE);
    }
    while (state < sense_values[upTo - 1]) {
      stop_all();
      while (state < sense_values[0] && upTo >= 1) {
        state = analog_grab(request);
        if (!spinning[0]) {
          pwm.setPWM(0, 0, speed_values[0]); //Spin 5th digit
          spinning[0] = 1;
        }
      }
      stop_all();
      while (state < sense_values[1] && state > sense_values[0]  && upTo >= 2) {
        state = analog_grab(request);
        if (!spinning[1]) {
          pwm.setPWM(2, 0, speed_values[1]); //Spin 5th digit
          spinning[1] = 1;
        }
      }
      stop_all();
      while (state < sense_values[2]  && state > sense_values[1]  && upTo >= 3) {
        state = analog_grab(request);
        if (!spinning[2]) {
          pwm.setPWM(4, 0, speed_values[2]); //Spin 5th digit
          spinning[2] = 1;
        }
      }
      stop_all();
      while (state < sense_values[3]  && state > sense_values[2]  && upTo >= 4) {
        state = analog_grab(request);
        if (!spinning[3]) {
          pwm.setPWM(6, 0, speed_values[3]); //Spin 5th digit
          spinning[3] = 1;
        }
      }
      stop_all();
      while (state < sense_values[4]  && state > sense_values[3]  && upTo == 5) {
        state = analog_grab(request);
        if (!spinning[4]) {
          pwm.setPWM(15, 0, speed_values[4]); //Spin 5th digit
          spinning[4] = 1;
        }
      }
      stop_all();
    }
    if (request) {
      for (byte i = 0; i < upTo; i++) {
        values[i] = 1;
      }
    } else {
      for (byte i = 0; i < upTo; i++) {
        values[i] = 0;
      }
    }
  }
}

void display_value(long input) {
  if (input != current_num) {
    int digits[5] = {input % 10, (input % 100) / 10, (input % 1000) / 100, (input % 10000) / 1000, (input % 100000) / 10000};
    digitalWrite(A3, LOW);
    for (byte i = 0; i < 5; i++) {
      pixels.setPixelColor(i, pixels.Color(0, 0, 0)); // Moderately bright green color.
    }
    pixels.show();
    if (input) {
      for (byte i = 0; i < 5; i++) {
        if (digits[i] != values[i]) {
          pwm.setPWM(servo[4 - i], 0, speed_values[4 - i]);
          spinning[4 - i] = 1;
          int spin_count = 0;
          if (digits[i] > values[i]) {
            spin_count = digits[i] - values[i];
          } else {
            spin_count = 10 + digits[i] - values[i];
          }
          Serial.println(spin_count);
          for (int spins = 0; spins < spin_count ; spins ++) {
            while (analogRead(DIGIT_SENSE) > sense_values[4 - i]);
            Serial.println("CLICK IN...");
            delay(10);
            while (analogRead(DIGIT_SENSE) < sense_values[4 - i]);
            delay(10);
            Serial.println("CLICK OUT...");
          }
          values[i] = digits[i];
        }
        stop_all();
      }
    } else {
      index_digits(1, 5);
      index_digits(0, 5);
    }
    Serial.println("DONE!");
    for (byte index = 0; index < 5; index++) {
      lit[index] = 1;
    }
    for (byte digit = 0; digit < 5; digit++) {
      if (digits[4 - digit] == 0) {
        lit[digit] = 0;
      } else {
        break;
      }
    }
    for (byte i = 0; i < 200; i++) {
      for (byte t = 0; t < 5; t++) {
        if (lit[t]) {
          pixels.setPixelColor(t, pixels.Color(i / R, i / G, i / B));
        }
      }
      pixels.show();
      delay(5);
    }
    current_num = input;
  } else if (input == 0) {
    index_digits(0, 5);
    current_num = 0;
  }
  digitalWrite(A3, HIGH);
}

void set_light_color(long setting) {
  switch (setting) {
    case 1000000: R = 1; G = 255; B = 255; break; //Red
    case 2000000: R = 255; G = 1; B = 255; break; //Green
    case 3000000: R = 255; G = 255; B = 1; break; //Blue
    case 4000000: R = 1; G = 1; B = 255; break; //Yellow
    case 5000000: R = 255; G = 1; B = 1; break; //Cyan
    case 6000000: R = 1; G = 255; B = 1; break; //Magenta
    case 7000000: R = 1; G = 1; B = 3; break; //Whtie
    case 8000000: R = 255; G = 255; B = 255; break; //OFF
  }
  for (byte i = 0; i < 200; i++) {
    for (byte t = 0; t < 5; t++) {
      if (lit[t]) {
        pixels.setPixelColor(t, pixels.Color(i / R, i / G, i / B));
      }
    }
    pixels.show();
    delay(5);
  }
  EEPROM.write(0, R);
  EEPROM.write(1, G);
  EEPROM.write(2, B);
}
