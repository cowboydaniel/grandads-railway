/*
 * Railway Crossing Lights Controller using ACS712 Block Detection
 *
 * Two ACS712 current-sensing blocks (one on each side of the crossing)
 * provide occupancy information. As soon as a train is detected on either
 * approach the crossing lights begin flashing. The lights remain active until
 * the train clears the far-side sensor, ensuring the crossing stays protected
 * while the train traverses the crossing. If a train reverses out of the block
 * without reaching the second sensor, a configurable timeout will eventually
 * release the crossing after both blocks have been clear for a period.
 */

#include <Arduino.h>

// -----------------------------------------------------------------------------
// PIN DEFINITIONS
// -----------------------------------------------------------------------------

const int SENSOR_WEST_PIN = A0;  // ACS712 sensor protecting the western approach
const int SENSOR_EAST_PIN = A1;  // ACS712 sensor protecting the eastern approach

const int LIGHT_LEFT_PIN = 9;    // Output driving the left flasher lamp
const int LIGHT_RIGHT_PIN = 10;  // Output driving the right flasher lamp

// -----------------------------------------------------------------------------
// CONFIGURATION CONSTANTS
// -----------------------------------------------------------------------------

const unsigned long SENSOR_SAMPLE_INTERVAL_MS = 25;   // Minimum interval between sensor reads
const unsigned long SENSOR_DEBOUNCE_MS = 60;           // Debounce for occupancy transitions
const int CALIBRATION_SAMPLES = 400;                   // Samples taken during zero-current calibration
const int OCCUPANCY_THRESHOLD = 30;                    // Raw ADC delta from baseline indicating current draw
const int BASELINE_TRACKING_DIVISOR = 64;              // Slow baseline tracking when block is clear

const unsigned long FLASH_INTERVAL_MS = 500;           // Alternating flash interval for the lights
const unsigned long RELEASE_TIMEOUT_MS = 10000;        // Fail-safe release if train backs away

const int LIGHT_ACTIVE_STATE = HIGH;                   // HIGH enables the LED driver or relay
const int LIGHT_INACTIVE_STATE = LOW;                  // LOW disables the LED driver or relay

// -----------------------------------------------------------------------------
// DATA STRUCTURES
// -----------------------------------------------------------------------------

typedef struct {
  const char* label;
  int pin;
  int baseline;
  int threshold;
  bool occupied;
  bool rawState;
  unsigned long lastSample;
  unsigned long lastDebounceChange;
} ACS712Sensor;

ACS712Sensor sensors[] = {
  {"West Approach", SENSOR_WEST_PIN, 0, OCCUPANCY_THRESHOLD, false, false, 0, 0},
  {"East Approach", SENSOR_EAST_PIN, 0, OCCUPANCY_THRESHOLD, false, false, 0, 0},
};

const size_t SENSOR_COUNT = sizeof(sensors) / sizeof(sensors[0]);

enum ApproachDirection {
  APPROACH_NONE,
  APPROACH_FROM_WEST,
  APPROACH_FROM_EAST
};

// -----------------------------------------------------------------------------
// STATE VARIABLES
// -----------------------------------------------------------------------------

bool crossingActive = false;
ApproachDirection activeDirection = APPROACH_NONE;
bool farSensorSeen = false;
unsigned long lastOccupancyActivity = 0;

bool leftLampOn = false;
unsigned long lastFlashToggle = 0;

// -----------------------------------------------------------------------------
// FORWARD DECLARATIONS
// -----------------------------------------------------------------------------

void calibrateSensors();
bool updateSensor(ACS712Sensor& sensor, unsigned long now);
void refreshCrossingState(bool westOccupied, bool eastOccupied, unsigned long now);
void activateCrossing(unsigned long now, ApproachDirection direction, bool otherSensorAlreadyActive);
void deactivateCrossing();
void updateLights(unsigned long now);

// -----------------------------------------------------------------------------
// SETUP
// -----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  pinMode(LIGHT_LEFT_PIN, OUTPUT);
  pinMode(LIGHT_RIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_LEFT_PIN, LIGHT_INACTIVE_STATE);
  digitalWrite(LIGHT_RIGHT_PIN, LIGHT_INACTIVE_STATE);

  calibrateSensors();

  Serial.println(F("Railway crossing controller ready."));
}

// -----------------------------------------------------------------------------
// MAIN LOOP
// -----------------------------------------------------------------------------

void loop() {
  unsigned long now = millis();

  bool westOccupied = updateSensor(sensors[0], now);
  bool eastOccupied = updateSensor(sensors[1], now);

  refreshCrossingState(westOccupied, eastOccupied, now);
  updateLights(now);
}

// -----------------------------------------------------------------------------
// SENSOR HANDLING
// -----------------------------------------------------------------------------

void calibrateSensors() {
  Serial.println(F("Calibrating ACS712 sensors..."));
  for (size_t i = 0; i < SENSOR_COUNT; ++i) {
    long accumulator = 0;
    for (int sample = 0; sample < CALIBRATION_SAMPLES; ++sample) {
      accumulator += analogRead(sensors[i].pin);
      delay(2);
    }
    sensors[i].baseline = accumulator / CALIBRATION_SAMPLES;
    sensors[i].lastSample = millis();
    sensors[i].lastDebounceChange = sensors[i].lastSample;

    Serial.print(F("  "));
    Serial.print(sensors[i].label);
    Serial.print(F(" baseline: "));
    Serial.println(sensors[i].baseline);
  }
}

bool updateSensor(ACS712Sensor& sensor, unsigned long now) {
  if ((now - sensor.lastSample) < SENSOR_SAMPLE_INTERVAL_MS) {
    return sensor.occupied;
  }

  sensor.lastSample = now;

  int reading = analogRead(sensor.pin);
  int delta = reading - sensor.baseline;
  bool rawState = abs(delta) >= sensor.threshold;

  if (!rawState) {
    // Track baseline slowly when no train current is detected.
    int adjustment = (sensor.baseline - reading) / BASELINE_TRACKING_DIVISOR;
    sensor.baseline -= adjustment;
  }

  if (rawState != sensor.rawState) {
    sensor.rawState = rawState;
    sensor.lastDebounceChange = now;
  }

  if ((now - sensor.lastDebounceChange) >= SENSOR_DEBOUNCE_MS && sensor.occupied != sensor.rawState) {
    sensor.occupied = sensor.rawState;

    Serial.print(sensor.label);
    Serial.print(F(" occupancy -> "));
    Serial.println(sensor.occupied ? F("DETECTED") : F("CLEAR"));
  }

  return sensor.occupied;
}

// -----------------------------------------------------------------------------
// CROSSING LOGIC
// -----------------------------------------------------------------------------

void refreshCrossingState(bool westOccupied, bool eastOccupied, unsigned long now) {
  bool anyOccupied = westOccupied || eastOccupied;
  if (anyOccupied) {
    lastOccupancyActivity = now;
  }

  if (!crossingActive) {
    if (westOccupied) {
      activateCrossing(now, APPROACH_FROM_WEST, eastOccupied);
    } else if (eastOccupied) {
      activateCrossing(now, APPROACH_FROM_EAST, westOccupied);
    }
    return;
  }

  // Update whether the train has reached the far-side sensor.
  if (!farSensorSeen) {
    if ((activeDirection == APPROACH_FROM_WEST && eastOccupied) ||
        (activeDirection == APPROACH_FROM_EAST && westOccupied)) {
      farSensorSeen = true;
      Serial.println(F("Far-side sensor detected. Crossing will release when clear."));
    }
  }

  if (!westOccupied && !eastOccupied) {
    if (farSensorSeen) {
      Serial.println(F("Both sensors clear after traversal. Crossing releasing."));
      deactivateCrossing();
    } else if ((now - lastOccupancyActivity) >= RELEASE_TIMEOUT_MS) {
      Serial.println(F("Release timeout reached with no second sensor detection. Crossing releasing."));
      deactivateCrossing();
    }
  }
}

void activateCrossing(unsigned long now, ApproachDirection direction, bool otherSensorAlreadyActive) {
  crossingActive = true;
  activeDirection = direction;
  farSensorSeen = otherSensorAlreadyActive;
  lastFlashToggle = now;
  leftLampOn = true;
  digitalWrite(LIGHT_LEFT_PIN, LIGHT_ACTIVE_STATE);
  digitalWrite(LIGHT_RIGHT_PIN, LIGHT_INACTIVE_STATE);

  Serial.print(F("Crossing activated from "));
  Serial.println(direction == APPROACH_FROM_WEST ? F("WEST") : F("EAST"));

  if (farSensorSeen) {
    Serial.println(F("Both sensors already active at activation."));
  }
}

void deactivateCrossing() {
  crossingActive = false;
  activeDirection = APPROACH_NONE;
  farSensorSeen = false;
  digitalWrite(LIGHT_LEFT_PIN, LIGHT_INACTIVE_STATE);
  digitalWrite(LIGHT_RIGHT_PIN, LIGHT_INACTIVE_STATE);
  leftLampOn = false;

  Serial.println(F("Crossing deactivated."));
}

// -----------------------------------------------------------------------------
// LIGHT CONTROL
// -----------------------------------------------------------------------------

void updateLights(unsigned long now) {
  if (!crossingActive) {
    return;
  }

  if ((now - lastFlashToggle) >= FLASH_INTERVAL_MS) {
    lastFlashToggle = now;
    leftLampOn = !leftLampOn;

    digitalWrite(LIGHT_LEFT_PIN, leftLampOn ? LIGHT_ACTIVE_STATE : LIGHT_INACTIVE_STATE);
    digitalWrite(LIGHT_RIGHT_PIN, leftLampOn ? LIGHT_INACTIVE_STATE : LIGHT_ACTIVE_STATE);
  }
}
