/*
  Corridor Lighting Control — ATmega328P / Arduino Uno, 16 MHz.
  Proteus wiring:
    PC0 / ADC0 (A0, physical Proteus pin 23) <- LDR divider;
    PD2 (D2, physical pin 32 in Proteus symbol) <- LOGICSTATE motion detector;
    PD4 (D4, physical pin 2 in Proteus symbol) <- button to GND;
    PB1 / OC1A (D9, physical pin 13 in Proteus symbol) -> R3 220 ohms -> MOSFET gate.

  Modes (press button to cycle): AUTO -> ALWAYS_ON -> ALWAYS_OFF -> AUTO.
  AUTO: daytime OFF; dark empty corridor 20%; dark + motion 100%,
  then 30s delay and smooth dimming to 20%.
  LDR divider: +5V -> 24k -> ADC0 -> LDR -> GND.
  Typical ADC: ~176 for 5k LDR (light), ~692 for 50k (dark).
*/

const uint8_t PIN_LDR = A0;
const uint8_t PIN_MOTION = 2;
const uint8_t PIN_BUTTON = 4;
const uint8_t PIN_LIGHT_PWM = 9;

const int THRESHOLD_DARK = 620; // ADC >= 620: darkness detected
const int THRESHOLD_LIGHT = 500; // ADC <= 500: daylight detected
const uint8_t BRIGHTNESS_STANDBY = 51; // 20% of 255
const unsigned long MOTION_HOLD_MS = 30000UL;
const unsigned long FADE_STEP_MS = 8UL;
const unsigned long DEBOUNCE_MS = 40UL;

enum OperatingMode : uint8_t { AUTO = 0, ALWAYS_ON = 1, ALWAYS_OFF = 2 };
OperatingMode mode = AUTO;

bool isDark = false;
bool motionEverSeen = false;
unsigned long lastMotionMs = 0;
uint8_t actualBrightness = 0;
unsigned long lastFadeStepMs = 0;

bool lastButtonSample = HIGH;
bool stableButtonState = HIGH;
unsigned long lastButtonChangeMs = 0;

void setup() {
  pinMode(PIN_MOTION, INPUT);        // LOGICSTATE provides a digital 0/1
  pinMode(PIN_BUTTON, INPUT_PULLUP); // momentary button connects to GND
  pinMode(PIN_LIGHT_PWM, OUTPUT);
  analogWrite(PIN_LIGHT_PWM, 0);
}

void loop() {
  const unsigned long now = millis();

  // LDR: higher ADC values correspond to darkness in this divider.
  const int lightAdc = analogRead(PIN_LDR);
  if (!isDark && lightAdc >= THRESHOLD_DARK) {
    isDark = true;
  } else if (isDark && lightAdc <= THRESHOLD_LIGHT) {
    isDark = false;
  }

  // Refresh the last activity time as long as motion is detected.
  if (digitalRead(PIN_MOTION) == HIGH) {
    motionEverSeen = true;
    lastMotionMs = now;
  }

  // Debounce and cycle between AUTO, ALWAYS_ON and ALWAYS_OFF.
  const bool buttonSample = digitalRead(PIN_BUTTON);
  if (buttonSample != lastButtonSample) {
    lastButtonChangeMs = now;
  }
  if ((now - lastButtonChangeMs) >= DEBOUNCE_MS &&
      buttonSample != stableButtonState) {
    stableButtonState = buttonSample;
    if (stableButtonState == LOW) {
      mode = static_cast<OperatingMode>((static_cast<uint8_t>(mode) + 1) % 3);
    }
  }
  lastButtonSample = buttonSample;

  uint8_t targetBrightness = 0;
  if (mode == ALWAYS_ON) {
    targetBrightness = 255;
  } else if (mode == ALWAYS_OFF || !isDark) {
    targetBrightness = 0;
  } else if (motionEverSeen && (now - lastMotionMs) < MOTION_HOLD_MS) {
    targetBrightness = 255;
  } else {
    targetBrightness = BRIGHTNESS_STANDBY;
  }

  // Gradual light transitions: 1 PWM step every 8 ms, no delay().
  if ((now - lastFadeStepMs) >= FADE_STEP_MS) {
    lastFadeStepMs = now;
    if (actualBrightness < targetBrightness) {
      ++actualBrightness;
    } else if (actualBrightness > targetBrightness) {
      --actualBrightness;
    }
    analogWrite(PIN_LIGHT_PWM, actualBrightness);
  }
}
