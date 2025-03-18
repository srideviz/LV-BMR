/********************
   Updated Arduino code for a one-shot push-button startup
   sequence with a 3-second buzzer, TSAL, and SSR activation.
   This version ignores push button events during the first
   500ms after power-up to avoid false triggers.
********************/

// Pin definitions
const int pinTSAL       = 7;    // TSAL indicator (light)
const int pinSSR        = 5;    // SSR/DC-DC enable signal
const int pinBuzzer     = 3;    // Buzzer

const int pinBrake      = A0;   // Brake sensor (shows 12V when brake ON, scaled to ~2.2V)
const int pinChgDetect  = A2;   // Charging detection (12V scaled down)
const int pinNeutral    = A4;   // Neutral sensor
const int pinPushButton = A5;   // Startup push button

// Thresholds (adjust to match your actual hardware levels)
// When brake is ON, voltage is ~2.2V, so a threshold of 2 means "brake pressed" is true.
float brakePressedThreshold = 2.0;  
float chargingThreshold     = 2.0;  
float neutralThreshold      = 2.0;  

// Buzzer timing
unsigned long buzzerOnTime = 3000;  // Buzzer on duration in milliseconds
unsigned long buzzerStartMillis = 0;

// Startup state and button edge detection
bool startupInitiated = false;
int previousPushState = HIGH;  // using INPUT_PULLUP, unpressed state is HIGH

// Variables to ignore spurious push events right after power-up.
unsigned long systemStartTime = 0;
unsigned long startupDebounceTime = 500;  // 500 ms delay at startup

void setup() {
  // Set digital pins as outputs
  pinMode(pinTSAL, OUTPUT);
  pinMode(pinSSR, OUTPUT);
  pinMode(pinBuzzer, OUTPUT);

  // Set analog/digital inputs
  pinMode(pinBrake, INPUT);
  pinMode(pinChgDetect, INPUT);
  pinMode(pinNeutral, INPUT);
  // For the push button, use internal pull-up so that unpressed reads HIGH.
  pinMode(pinPushButton, INPUT_PULLUP);

  // Initialize outputs to LOW
  digitalWrite(pinTSAL, LOW);
  digitalWrite(pinSSR, LOW);
  digitalWrite(pinBuzzer, LOW);

  // Start Serial for debugging
  Serial.begin(9600);
  
  // Record the system startup time
  systemStartTime = millis();
}

void loop() {
  // Read analog inputs and convert to voltage (assuming a 5V reference)
  float brakeVoltage     = analogRead(pinBrake)     * (5.0 / 1023.0);
  float chgDetectVoltage = analogRead(pinChgDetect) * (5.0 / 1023.0);
  float neutralVoltage   = analogRead(pinNeutral)   * (5.0 / 1023.0);

  // Determine sensor conditions.
  // For your brake sensor, when brake is ON the voltage is high (e.g. 2.2V),
  // so we set isBrakePressed true when the voltage is above the threshold.
  bool isBrakePressed = (brakeVoltage > brakePressedThreshold);
  bool isCharging     = (chgDetectVoltage > chargingThreshold);
  bool isNeutral      = (neutralVoltage > neutralThreshold);

  // Read push button state (INPUT_PULLUP: LOW means pressed)
  int currentPushState = digitalRead(pinPushButton);

  // Only consider push button transitions after the debounce period.
  bool pushEvent = false;
  if (millis() - systemStartTime > startupDebounceTime) {
    if (previousPushState == HIGH && currentPushState == LOW) {
      pushEvent = true;
    }
  }
  previousPushState = currentPushState;  // update for next loop

  // If a valid push event occurs and safe conditions are met, latch startup.
  // Note: We require that the push event occurs to trigger startup.
  if (!startupInitiated && pushEvent && !isCharging && isNeutral && isBrakePressed) {
    startupInitiated = true;
    buzzerStartMillis = millis();
    digitalWrite(pinBuzzer, HIGH);  // start the buzzer
    Serial.println("Startup initiated!");
  }

  // If startup is latched and safe conditions remain, keep outputs on.
  if (startupInitiated && !isCharging && isNeutral) {
    digitalWrite(pinTSAL, HIGH);
    digitalWrite(pinSSR, HIGH);

    // Keep the buzzer on for the designated duration.
    if (millis() - buzzerStartMillis >= buzzerOnTime) {
      digitalWrite(pinBuzzer, LOW);
    }
  } 
  else if (startupInitiated && !isCharging && !isNeutral) {
    // If for some reason neutral is lost, keep TSAL on as per your design.
    digitalWrite(pinTSAL, HIGH);
  } 
  else {
    // If any condition is not met, reset the startup sequence.
    startupInitiated = false;
    digitalWrite(pinTSAL, LOW);
    digitalWrite(pinSSR, LOW);
    digitalWrite(pinBuzzer, LOW);
  }

  // Debug prints for monitoring sensor values and startup status.
  Serial.print("Brake: ");
  Serial.print(brakeVoltage);
  Serial.print(" | Chg: ");
  Serial.print(chgDetectVoltage);
  Serial.print(" | Neutral: ");
  Serial.print(neutralVoltage);
  Serial.print(" | Startup: ");
  Serial.println(startupInitiated);

  delay(10);
}