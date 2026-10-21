/*
  Corridor Lighting Control - five independent zones
  MCU: ATmega328P @ 16 MHz, Arduino Uno core, Proteus 8

  Zone  PIR / input      PWM / output
   1    PD2 / D2          PB1 / D9
   2    PD7 / D7          PD3 / D3
   3    PB0 / D8          PD5 / D5
   4    PC1 / A1          PD6 / D6
   5    PC2 / A2          PB2 / D10

  LDR divider: PC0 / A0 (larger ADC value means darker)
  Emergency button: PD4 / D4 (button to GND, INPUT_PULLUP)

  Normal AUTO mode:
    Daylight                 -> all lights OFF
    Dark without motion     -> all lights at 30% PWM
    PIRn active             -> light n gradually brightens to 100%
    After motion timeout    -> light n gradually returns to 30%

  Emergency mode:
    First button press      -> ALL ON immediately at 100%
    Second button press     -> return to AUTO (with normal fading)
    Overrides the light sensor and every PIR input.
*/

#include <Arduino.h>

const uint8_t ZONE_COUNT = 5;
const uint8_t PIN_LDR = A0;
const uint8_t PIN_EMERGENCY_BUTTON = 4;

const uint8_t PIR_PINS[ZONE_COUNT] = {2, 7, 8, A1, A2};
const uint8_t PWM_PINS[ZONE_COUNT] = {9, 3, 5, 6, 10};

// Select true for the slow Proteus simulation, false for course-project timing.
const bool FAST_PROTEUS_DEMO = true;

// The LDR divider on the existing schematic raises ADC readings in darkness.
// Different threshold values provide hysteresis against flickering.
const int DARK_THRESHOLD = 620;
const int LIGHT_THRESHOLD = 500;

const uint8_t STANDBY_PWM = 77;  // 77 / 255 = 30.2% duty cycle

const unsigned long ZONE_HOLD_MS = FAST_PROTEUS_DEMO ? 1000UL : 8000UL;
const unsigned long FADE_STEP_MS = FAST_PROTEUS_DEMO ? 1UL : 8UL;
const unsigned long DEBOUNCE_MS = FAST_PROTEUS_DEMO ? 5UL : 40UL;

bool dark = false;
bool emergencyActive = false;

bool motionSeen[ZONE_COUNT] = {false, false, false, false, false};
unsigned long lastMotionMs[ZONE_COUNT] = {0, 0, 0, 0, 0};
uint8_t brightness[ZONE_COUNT] = {0, 0, 0, 0, 0};
unsigned long lastFadeMs = 0;

bool previousButtonSample = HIGH;
bool stableButtonState = HIGH;
unsigned long lastButtonTransitionMs = 0;

void setAllLightsNow(uint8_t pwmValue) {
  for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
    brightness[i] = pwmValue;
    analogWrite(PWM_PINS[i], pwmValue);
  }
}

void setup() {
  pinMode(PIN_EMERGENCY_BUTTON, INPUT_PULLUP);

  for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
    pinMode(PIR_PINS[i], INPUT);  // Proteus LOGICSTATE supplies 0 or 1.
    pinMode(PWM_PINS[i], OUTPUT);
  }

  setAllLightsNow(0);
}

void loop() {
  const unsigned long now = millis();

  // Debounced button: toggle emergency mode only on a confirmed press.
  const bool buttonSample = digitalRead(PIN_EMERGENCY_BUTTON);
  if (buttonSample != previousButtonSample) {
    lastButtonTransitionMs = now;
  }
  if ((now - lastButtonTransitionMs) >= DEBOUNCE_MS &&
      buttonSample != stableButtonState) {
    stableButtonState = buttonSample;
    if (stableButtonState == LOW) {
      emergencyActive = !emergencyActive;
      lastFadeMs = now;
      if (emergencyActive) {
        // Emergency override: no fade, no dependence on day/night or PIRs.
        setAllLightsNow(255);
      }
    }
  }
  previousButtonSample = buttonSample;

  if (emergencyActive) {
    // Outputs remain at 100%; no need to simulate ADC/PIR reads here.
    return;
  }

  // Read ambient light and apply hysteresis.
  const int adc = analogRead(PIN_LDR);
  if (!dark && adc >= DARK_THRESHOLD) {
    dark = true;
  } else if (dark && adc <= LIGHT_THRESHOLD) {
    dark = false;
  }

  // Each zone uses its own motion timestamp.
  for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
    if (digitalRead(PIR_PINS[i]) == HIGH) {
      motionSeen[i] = true;
      lastMotionMs[i] = now;
    }
  }

  // Gradual change for each lamp in automatic mode.
  if ((now - lastFadeMs) >= FADE_STEP_MS) {
    lastFadeMs = now;

    for (uint8_t i = 0; i < ZONE_COUNT; ++i) {
      uint8_t target = 0;
      if (dark) {
        const bool zoneOccupied = motionSeen[i] &&
            ((now - lastMotionMs[i]) < ZONE_HOLD_MS);
        target = zoneOccupied ? 255 : STANDBY_PWM;
      }

      if (brightness[i] < target) {
        ++brightness[i];
        analogWrite(PWM_PINS[i], brightness[i]);
      } else if (brightness[i] > target) {
        --brightness[i];
        analogWrite(PWM_PINS[i], brightness[i]);
      }
    }
  }
}
