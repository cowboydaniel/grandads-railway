/*
 * ACS712 Output Test Code
 *
 * This test program validates the ACS712 current sensor outputs and
 * crossing light control functionality. It provides diagnostic information
 * and allows testing of various scenarios.
 *
 * Test Modes:
 * 1. Raw Sensor Reading Test - Displays continuous ADC values from both sensors
 * 2. Threshold Detection Test - Shows when current exceeds detection threshold
 * 3. Light Output Test - Cycles through all light states
 * 4. Baseline Calibration Test - Validates sensor calibration
 * 5. Full System Test - Runs complete crossing activation/deactivation cycle
 */

#include <Arduino.h>

// -----------------------------------------------------------------------------
// PIN DEFINITIONS (Match main controller)
// -----------------------------------------------------------------------------

const int SENSOR_WEST_PIN = A0;
const int SENSOR_EAST_PIN = A1;
const int LIGHT_LEFT_PIN = 9;
const int LIGHT_RIGHT_PIN = 10;

// -----------------------------------------------------------------------------
// TEST CONFIGURATION
// -----------------------------------------------------------------------------

const int CALIBRATION_SAMPLES = 400;
const int OCCUPANCY_THRESHOLD = 5;  // Reduced from 30 based on observed train signals (+4 to +8)
const unsigned long SENSOR_READ_INTERVAL = 100;  // Display update rate for continuous monitoring
const int LIGHT_ACTIVE_STATE = HIGH;
const int LIGHT_INACTIVE_STATE = LOW;

// -----------------------------------------------------------------------------
// TEST DATA
// -----------------------------------------------------------------------------

struct SensorData {
  const char* label;
  int pin;
  int baseline;
  int currentReading;
  int delta;
  bool thresholdExceeded;
};

SensorData sensors[] = {
  {"West", SENSOR_WEST_PIN, 0, 0, 0, false},
  {"East", SENSOR_EAST_PIN, 0, 0, 0, false}
};

const size_t SENSOR_COUNT = sizeof(sensors) / sizeof(sensors[0]);

// -----------------------------------------------------------------------------
// MENU SYSTEM
// -----------------------------------------------------------------------------

void printMenu() {
  Serial.println(F("\n========================================"));
  Serial.println(F("  ACS712 OUTPUT TEST MENU"));
  Serial.println(F("========================================"));
  Serial.println(F("1. Raw Sensor Reading Test"));
  Serial.println(F("2. Threshold Detection Test"));
  Serial.println(F("3. Light Output Test"));
  Serial.println(F("4. Baseline Calibration Test"));
  Serial.println(F("5. Full System Integration Test"));
  Serial.println(F("6. Continuous Monitor Mode"));
  Serial.println(F("7. Display Current Baselines"));
  Serial.println(F("0. Show this menu"));
  Serial.println(F("========================================"));
  Serial.println(F("Enter test number:"));
}

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    ; // Wait for serial port to connect (up to 3 seconds)
  }

  Serial.println(F("\n\n=== ACS712 Output Test Program ==="));
  Serial.println(F("Initializing..."));

  // Configure pins
  pinMode(LIGHT_LEFT_PIN, OUTPUT);
  pinMode(LIGHT_RIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_LEFT_PIN, LIGHT_INACTIVE_STATE);
  digitalWrite(LIGHT_RIGHT_PIN, LIGHT_INACTIVE_STATE);

  // Initial calibration
  calibrateSensors();

  Serial.println(F("Initialization complete."));
  printMenu();
}

// -----------------------------------------------------------------------------
// MAIN LOOP
// -----------------------------------------------------------------------------

void loop() {
  if (Serial.available() > 0) {
    char input = Serial.read();

    // Clear any remaining input
    while (Serial.available() > 0) {
      Serial.read();
    }

    switch (input) {
      case '1':
        testRawSensorReadings();
        break;
      case '2':
        testThresholdDetection();
        break;
      case '3':
        testLightOutputs();
        break;
      case '4':
        testCalibration();
        break;
      case '5':
        testFullSystem();
        break;
      case '6':
        continuousMonitor();
        break;
      case '7':
        displayBaselines();
        break;
      case '0':
        printMenu();
        break;
      default:
        Serial.println(F("Invalid option. Press 0 for menu."));
        break;
    }
  }
}

// -----------------------------------------------------------------------------
// CALIBRATION
// -----------------------------------------------------------------------------

void calibrateSensors() {
  Serial.println(F("\n--- Calibrating Sensors ---"));
  Serial.println(F("Ensure no current is flowing through sensors..."));
  delay(1000);

  for (size_t i = 0; i < SENSOR_COUNT; ++i) {
    long accumulator = 0;
    Serial.print(F("Calibrating "));
    Serial.print(sensors[i].label);
    Serial.print(F(" sensor ("));
    Serial.print(CALIBRATION_SAMPLES);
    Serial.println(F(" samples)..."));

    for (int sample = 0; sample < CALIBRATION_SAMPLES; ++sample) {
      accumulator += analogRead(sensors[i].pin);
      delay(2);

      // Progress indicator
      if (sample % 100 == 0) {
        Serial.print(F("."));
      }
    }
    Serial.println();

    sensors[i].baseline = accumulator / CALIBRATION_SAMPLES;

    Serial.print(F("  Baseline: "));
    Serial.print(sensors[i].baseline);
    Serial.print(F(" ("));
    Serial.print((sensors[i].baseline * 5.0) / 1023.0, 3);
    Serial.println(F("V)"));
  }
  Serial.println(F("Calibration complete.\n"));
}

void displayBaselines() {
  Serial.println(F("\n--- Current Baselines ---"));
  for (size_t i = 0; i < SENSOR_COUNT; ++i) {
    Serial.print(sensors[i].label);
    Serial.print(F(": "));
    Serial.print(sensors[i].baseline);
    Serial.print(F(" ADC ("));
    Serial.print((sensors[i].baseline * 5.0) / 1023.0, 3);
    Serial.println(F("V)"));
  }
  Serial.println(F("Threshold: "));
  Serial.print(F("  "));
  Serial.print(OCCUPANCY_THRESHOLD);
  Serial.println(F(" ADC units from baseline"));
  Serial.println();
}

// -----------------------------------------------------------------------------
// TEST 1: RAW SENSOR READINGS
// -----------------------------------------------------------------------------

void testRawSensorReadings() {
  Serial.println(F("\n=== Test 1: Raw Sensor Readings ==="));
  Serial.println(F("Displaying continuous ADC values."));
  Serial.println(F("Send any character to stop.\n"));
  delay(1000);

  Serial.println(F("Time(ms)\tWest_ADC\tWest_V\t\tEast_ADC\tEast_V"));
  Serial.println(F("--------\t--------\t------\t\t--------\t------"));

  // Clear any stray characters from serial buffer before starting
  while (Serial.available()) {
    Serial.read();
  }

  while (!Serial.available()) {
    unsigned long now = millis();
    int westReading = analogRead(SENSOR_WEST_PIN);
    int eastReading = analogRead(SENSOR_EAST_PIN);

    Serial.print(now);
    Serial.print(F("\t\t"));
    Serial.print(westReading);
    Serial.print(F("\t\t"));
    Serial.print((westReading * 5.0) / 1023.0, 3);
    Serial.print(F("\t\t"));
    Serial.print(eastReading);
    Serial.print(F("\t\t"));
    Serial.println((eastReading * 5.0) / 1023.0, 3);

    delay(SENSOR_READ_INTERVAL);
  }

  // Clear input
  while (Serial.available()) Serial.read();
  Serial.println(F("\nTest complete.\n"));
}

// -----------------------------------------------------------------------------
// TEST 2: THRESHOLD DETECTION
// -----------------------------------------------------------------------------

void testThresholdDetection() {
  Serial.println(F("\n=== Test 2: Threshold Detection ==="));
  Serial.println(F("Monitoring for current above threshold."));
  Serial.println(F("Send any character to stop.\n"));
  delay(1000);

  Serial.println(F("Time(ms)\tSensor\tReading\tBaseline\tDelta\tDetected"));
  Serial.println(F("--------\t------\t-------\t--------\t-----\t--------"));

  // Clear any stray characters from serial buffer before starting
  while (Serial.available()) {
    Serial.read();
  }

  while (!Serial.available()) {
    unsigned long now = millis();

    for (size_t i = 0; i < SENSOR_COUNT; ++i) {
      sensors[i].currentReading = analogRead(sensors[i].pin);
      sensors[i].delta = sensors[i].currentReading - sensors[i].baseline;
      sensors[i].thresholdExceeded = abs(sensors[i].delta) >= OCCUPANCY_THRESHOLD;

      Serial.print(now);
      Serial.print(F("\t\t"));
      Serial.print(sensors[i].label);
      Serial.print(F("\t"));
      Serial.print(sensors[i].currentReading);
      Serial.print(F("\t"));
      Serial.print(sensors[i].baseline);
      Serial.print(F("\t\t"));
      Serial.print(sensors[i].delta);
      Serial.print(F("\t"));
      Serial.println(sensors[i].thresholdExceeded ? F("YES ***") : F("no"));
    }

    Serial.println();
    delay(SENSOR_READ_INTERVAL);
  }

  // Clear input
  while (Serial.available()) Serial.read();
  Serial.println(F("\nTest complete.\n"));
}

// -----------------------------------------------------------------------------
// TEST 3: LIGHT OUTPUT TEST
// -----------------------------------------------------------------------------

void testLightOutputs() {
  Serial.println(F("\n=== Test 3: Light Output Test ==="));
  Serial.println(F("Testing all light output states...\n"));

  // Test 1: Both Off
  Serial.println(F("State 1: Both lights OFF"));
  digitalWrite(LIGHT_LEFT_PIN, LIGHT_INACTIVE_STATE);
  digitalWrite(LIGHT_RIGHT_PIN, LIGHT_INACTIVE_STATE);
  Serial.println(F("  Left: OFF, Right: OFF"));
  delay(2000);

  // Test 2: Left On
  Serial.println(F("State 2: Left ON, Right OFF"));
  digitalWrite(LIGHT_LEFT_PIN, LIGHT_ACTIVE_STATE);
  digitalWrite(LIGHT_RIGHT_PIN, LIGHT_INACTIVE_STATE);
  Serial.println(F("  Left: ON, Right: OFF"));
  delay(2000);

  // Test 3: Right On
  Serial.println(F("State 3: Left OFF, Right ON"));
  digitalWrite(LIGHT_LEFT_PIN, LIGHT_INACTIVE_STATE);
  digitalWrite(LIGHT_RIGHT_PIN, LIGHT_ACTIVE_STATE);
  Serial.println(F("  Left: OFF, Right: ON"));
  delay(2000);

  // Test 4: Both On
  Serial.println(F("State 4: Both lights ON"));
  digitalWrite(LIGHT_LEFT_PIN, LIGHT_ACTIVE_STATE);
  digitalWrite(LIGHT_RIGHT_PIN, LIGHT_ACTIVE_STATE);
  Serial.println(F("  Left: ON, Right: ON"));
  delay(2000);

  // Test 5: Alternating Flash
  Serial.println(F("State 5: Alternating Flash (10 cycles)"));
  for (int i = 0; i < 10; ++i) {
    digitalWrite(LIGHT_LEFT_PIN, LIGHT_ACTIVE_STATE);
    digitalWrite(LIGHT_RIGHT_PIN, LIGHT_INACTIVE_STATE);
    Serial.println(F("  Left: ON, Right: OFF"));
    delay(500);

    digitalWrite(LIGHT_LEFT_PIN, LIGHT_INACTIVE_STATE);
    digitalWrite(LIGHT_RIGHT_PIN, LIGHT_ACTIVE_STATE);
    Serial.println(F("  Left: OFF, Right: ON"));
    delay(500);
  }

  // Return to safe state
  digitalWrite(LIGHT_LEFT_PIN, LIGHT_INACTIVE_STATE);
  digitalWrite(LIGHT_RIGHT_PIN, LIGHT_INACTIVE_STATE);
  Serial.println(F("\nAll lights OFF - Test complete.\n"));
}

// -----------------------------------------------------------------------------
// TEST 4: CALIBRATION TEST
// -----------------------------------------------------------------------------

void testCalibration() {
  Serial.println(F("\n=== Test 4: Baseline Calibration Test ==="));

  // Store old baselines
  int oldBaselines[SENSOR_COUNT];
  for (size_t i = 0; i < SENSOR_COUNT; ++i) {
    oldBaselines[i] = sensors[i].baseline;
  }

  // Recalibrate
  calibrateSensors();

  // Compare
  Serial.println(F("--- Baseline Comparison ---"));
  Serial.println(F("Sensor\t\tOld\tNew\tDrift"));
  for (size_t i = 0; i < SENSOR_COUNT; ++i) {
    int drift = sensors[i].baseline - oldBaselines[i];
    Serial.print(sensors[i].label);
    Serial.print(F("\t\t"));
    Serial.print(oldBaselines[i]);
    Serial.print(F("\t"));
    Serial.print(sensors[i].baseline);
    Serial.print(F("\t"));
    Serial.println(drift);
  }

  Serial.println(F("\nCalibration test complete.\n"));
}

// -----------------------------------------------------------------------------
// TEST 5: FULL SYSTEM TEST
// -----------------------------------------------------------------------------

void testFullSystem() {
  Serial.println(F("\n=== Test 5: Full System Integration Test ==="));
  Serial.println(F("This test simulates a complete crossing activation cycle."));
  Serial.println(F("You should trigger the west sensor, then the east sensor.\n"));

  bool westTriggered = false;
  bool eastTriggered = false;
  bool crossingActive = false;
  unsigned long activationTime = 0;

  Serial.println(F("Waiting for West sensor activation..."));

  while (true) {
    // Read sensors
    for (size_t i = 0; i < SENSOR_COUNT; ++i) {
      sensors[i].currentReading = analogRead(sensors[i].pin);
      sensors[i].delta = sensors[i].currentReading - sensors[i].baseline;
      sensors[i].thresholdExceeded = abs(sensors[i].delta) >= OCCUPANCY_THRESHOLD;
    }

    // West sensor detection
    if (!westTriggered && sensors[0].thresholdExceeded) {
      westTriggered = true;
      crossingActive = true;
      activationTime = millis();
      Serial.println(F("\n*** WEST SENSOR TRIGGERED ***"));
      Serial.println(F("Crossing activated - lights flashing"));
      Serial.println(F("Waiting for East sensor activation..."));
    }

    // East sensor detection
    if (westTriggered && !eastTriggered && sensors[1].thresholdExceeded) {
      eastTriggered = true;
      unsigned long traversalTime = millis() - activationTime;
      Serial.println(F("\n*** EAST SENSOR TRIGGERED ***"));
      Serial.print(F("Train traversal time: "));
      Serial.print(traversalTime);
      Serial.println(F(" ms"));
      Serial.println(F("Waiting for both sensors to clear..."));
    }

    // Update lights if crossing is active
    if (crossingActive) {
      unsigned long now = millis();
      bool leftOn = ((now / 500) % 2) == 0;
      digitalWrite(LIGHT_LEFT_PIN, leftOn ? LIGHT_ACTIVE_STATE : LIGHT_INACTIVE_STATE);
      digitalWrite(LIGHT_RIGHT_PIN, leftOn ? LIGHT_INACTIVE_STATE : LIGHT_ACTIVE_STATE);
    }

    // Check for clear condition
    if (westTriggered && eastTriggered && !sensors[0].thresholdExceeded && !sensors[1].thresholdExceeded) {
      unsigned long totalTime = millis() - activationTime;
      Serial.println(F("\n*** BOTH SENSORS CLEAR ***"));
      Serial.println(F("Crossing deactivated - lights off"));
      Serial.print(F("Total activation time: "));
      Serial.print(totalTime);
      Serial.println(F(" ms"));

      digitalWrite(LIGHT_LEFT_PIN, LIGHT_INACTIVE_STATE);
      digitalWrite(LIGHT_RIGHT_PIN, LIGHT_INACTIVE_STATE);

      Serial.println(F("\nFull system test complete.\n"));
      break;
    }

    // Allow user to abort
    if (Serial.available()) {
      while (Serial.available()) Serial.read();
      Serial.println(F("\nTest aborted by user.\n"));
      digitalWrite(LIGHT_LEFT_PIN, LIGHT_INACTIVE_STATE);
      digitalWrite(LIGHT_RIGHT_PIN, LIGHT_INACTIVE_STATE);
      break;
    }

    delay(50);
  }
}

// -----------------------------------------------------------------------------
// TEST 6: CONTINUOUS MONITOR
// -----------------------------------------------------------------------------

void continuousMonitor() {
  Serial.println(F("\n=== Test 6: Continuous Monitor Mode ==="));
  Serial.println(F("Real-time display of all sensor and output states."));
  Serial.println(F("Lights will activate automatically when train detected."));
  Serial.println(F("Send any character to stop.\n"));
  delay(1000);

  // Clear any stray characters from serial buffer before starting
  while (Serial.available()) {
    Serial.read();
  }

  bool crossingActive = false;

  while (!Serial.available()) {
    unsigned long now = millis();

    // Read sensors
    for (size_t i = 0; i < SENSOR_COUNT; ++i) {
      sensors[i].currentReading = analogRead(sensors[i].pin);
      sensors[i].delta = sensors[i].currentReading - sensors[i].baseline;
      sensors[i].thresholdExceeded = abs(sensors[i].delta) >= OCCUPANCY_THRESHOLD;
    }

    // Determine if any sensor is detecting
    bool anyDetection = sensors[0].thresholdExceeded || sensors[1].thresholdExceeded;

    // Update crossing state
    if (anyDetection && !crossingActive) {
      crossingActive = true;
      Serial.println(F("\n*** CROSSING ACTIVATED ***"));
    } else if (!anyDetection && crossingActive) {
      crossingActive = false;
      digitalWrite(LIGHT_LEFT_PIN, LIGHT_INACTIVE_STATE);
      digitalWrite(LIGHT_RIGHT_PIN, LIGHT_INACTIVE_STATE);
      Serial.println(F("\n*** CROSSING DEACTIVATED ***"));
    }

    // Control lights - alternating flash when active
    if (crossingActive) {
      bool leftOn = ((now / 500) % 2) == 0;
      digitalWrite(LIGHT_LEFT_PIN, leftOn ? LIGHT_ACTIVE_STATE : LIGHT_INACTIVE_STATE);
      digitalWrite(LIGHT_RIGHT_PIN, leftOn ? LIGHT_INACTIVE_STATE : LIGHT_ACTIVE_STATE);
    }

    // Read light states for display
    int leftState = digitalRead(LIGHT_LEFT_PIN);
    int rightState = digitalRead(LIGHT_RIGHT_PIN);

    // Display compact status
    Serial.print(F("["));
    Serial.print(now);
    Serial.print(F("] W:"));
    Serial.print(sensors[0].currentReading);
    Serial.print(F("("));
    Serial.print(sensors[0].delta > 0 ? "+" : "");
    Serial.print(sensors[0].delta);
    Serial.print(F(")"));
    Serial.print(sensors[0].thresholdExceeded ? F("[DET]") : F("[---]"));
    Serial.print(F(" E:"));
    Serial.print(sensors[1].currentReading);
    Serial.print(F("("));
    Serial.print(sensors[1].delta > 0 ? "+" : "");
    Serial.print(sensors[1].delta);
    Serial.print(F(")"));
    Serial.print(sensors[1].thresholdExceeded ? F("[DET]") : F("[---]"));
    Serial.print(F(" Lights: L="));
    Serial.print(leftState == LIGHT_ACTIVE_STATE ? F("ON ") : F("OFF"));
    Serial.print(F(" R="));
    Serial.println(rightState == LIGHT_ACTIVE_STATE ? F("ON ") : F("OFF"));

    delay(SENSOR_READ_INTERVAL);
  }

  // Clear input and turn off lights
  while (Serial.available()) Serial.read();
  digitalWrite(LIGHT_LEFT_PIN, LIGHT_INACTIVE_STATE);
  digitalWrite(LIGHT_RIGHT_PIN, LIGHT_INACTIVE_STATE);
  Serial.println(F("\nMonitor stopped.\n"));
}
