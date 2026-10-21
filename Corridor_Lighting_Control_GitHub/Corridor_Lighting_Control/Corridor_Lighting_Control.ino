/*
  Corridor Lighting Control: 5 independent zones
  ATmega328P, 16 MHz, Arduino Uno core, Proteus 8

  Zone   PIR (input)       PWM (output)
  1      PD2 / D2          PB1 / D9
  2      PD7 / D7          PD3 / D3
  3      PB0 / D8          PD5 / D5
  4      PC1 / A1          PD6 / D6
  5      PC2 / A2          PB2 / D10

  LDR divider -> PC0 / A0
  Button -> PD4 / D4 (other contact -> GND)

  AUTO:
    daylight: all lamps OFF;
    darkness, no movement: all lamps at 20%;
    PIRn detects motion: lamp n smoothly reaches 100%;
    after no motion for ZONE_HOLD_MS, that lamp returns to 20%.
  Button: AUTO -> ALL_ON -> ALL_OFF -> AUTO.
*/

#include <Arduino.h>

const uint8_t ZONE_COUNT = 5;
const uint8_t PIN_LDR = A0;
const uint8_t PIN_BUTTON = 4;
const uint8_t PIR_PINS[ZONE_COUNT] = {2, 7, 8, A1, A2};
const uint8_t PWM_PINS[ZONE_COUNT] = {9, 3, 5, 6, 10};

const int DARK_THRESHOLD = 620;   // ADC >= 620: darkness
const int LIGHT_THRESHOLD = 500;  // ADC <= 500: daylight
const uint8_t STANDBY_PWM = 51;   // 20% of 255
const unsigned long ZONE_HOLD_MS = 8000UL;  // 8 s; change to 30000UL for 30 s
const unsigned long FADE_STEP_MS = 8UL;     // ~2 seconds, 0 -> 100%
const unsigned long DEBOUNCE_MS = 40UL;

enum OperatingMode { MODE_AUTO, MODE_ALL_ON, MODE_ALL_OFF };
OperatingMode mode = MODE_AUTO;
bool dark = false;

bool motionSeen[ZONE_COUNT] = {false, false, false, false, false};
unsigned long lastMotionMs[ZONE_COUNT] = {0, 0, 0, 0, 0};
uint8_t brightness[ZONE_COUNT] = {0, 0, 0, 0, 0};
unsigned long lastFadeMs = 0;

bool previousButtonSample = HIGH;
bool stableButtonState = HIGH;
unsigned long lastButtonTransitionMs = 0;

void setup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
    pinMode(PIR_PINS[i], INPUT);     // Proteus LOGICSTATE supplies 0/1
    pinMode(PWM_PINS[i], OUTPUT);
    analogWrite(PWM_PINS[i], 0);
  }
}

void loop() {
  const unsigned long now = millis();

  // Hysteresis prevents flicker near the illumination threshold.
  const int adc = analogRead(PIN_LDR);
  if (!dark && adc >= DARK_THRESHOLD) {
    dark = true;
  } else if (dark && adc <= LIGHT_THRESHOLD) {
    dark = false;
  }

  // Motion timing is independent for every lighting zone.
  for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
    if (digitalRead(PIR_PINS[i]) == HIGH) {
      motionSeen[i] = true;
      lastMotionMs[i] = now;
    }
  }

  // One button switches between automatic, manual ON and manual OFF.
  const bool buttonSample = digitalRead(PIN_BUTTON);
  if (buttonSample != previousButtonSample) {
    lastButtonTransitionMs = now;
  }
  if ((now - lastButtonTransitionMs) >= DEBOUNCE_MS &&
      buttonSample != stableButtonState) {
    stableButtonState = buttonSample;
    if (stableButtonState == LOW) {
      mode = static_cast<OperatingMode>((static_cast<uint8_t>(mode) + 1) % 3);
    }
  }
  previousButtonSample = buttonSample;

  // Smoothly move all PWM outputs toward their independent targets.
  if ((now - lastFadeMs) >= FADE_STEP_MS) {
    lastFadeMs = now;
    for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
      uint8_t target = 0;
      if (mode == MODE_ALL_ON) {
        target = 255;
      } else if (mode == MODE_ALL_OFF || !dark) {
        target = 0;
      } else if (motionSeen[i] && (now - lastMotionMs[i] < ZONE_HOLD_MS)) {
        target = 255;
      } else {
        target = STANDBY_PWM;
      }

      if (brightness[i] < target) {
        ++brightness[i];
      } else if (brightness[i] > target) {
        --brightness[i];
      }
      analogWrite(PWM_PINS[i], brightness[i]);
    }
  }
}

      analogWrite(PWM_PINS[i], brightness[i]);
    }
  }
}
