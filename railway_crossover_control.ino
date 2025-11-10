/*
 * Railway Control System - Coacting Crossover and Dual Y Junctions
 *
 * Controls three point motors from a single Arduino using LED feedback from
 * Tortoise motors. One coacting crossover blocks both lines when active, while
 * two independent Y junctions select which approach feeds Line 3. For each Y
 * junction the non-selected approach is killed by removing track power. All
 * sampling is non-blocking; relay outputs react immediately after a stable
 * sensor transition is detected.
 */

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// LED sensor inputs from Tortoise motors
const int CROSSOVER_SENSOR_PIN = A2;
const int Y1_SENSOR_PIN = A3;
const int Y2_SENSOR_PIN = A4;

// Relay outputs (Active LOW - relay energizes when pin is LOW)
const int RELAY_LINE1_RIGHT_TO_LEFT = 3;  // Coacting crossover - Line 1
const int RELAY_LINE2_LEFT_TO_RIGHT = 4;  // Coacting crossover - Line 2
const int RELAY_Y1_LINE1 = 5;             // Y Junction 1 - Line 1 feed
const int RELAY_Y1_LINE2 = 6;             // Y Junction 1 - Line 2 feed
const int RELAY_Y2_LINE1 = 7;             // Y Junction 2 - Line 1 feed
const int RELAY_Y2_LINE2 = 8;             // Y Junction 2 - Line 2 feed

// ============================================================================
// CONSTANTS
// ============================================================================

const int CROSSOVER_ACTIVE = LOW;    // Coacting crossover active (both blocked)
const int CROSSOVER_INACTIVE = HIGH; // Coacting crossover inactive (both free)

const int TRACK_POWER_ON = HIGH;     // Relay OFF (power flows to track)
const int TRACK_POWER_OFF = LOW;     // Relay ON (power removed from track)

const int ROUTE_LINE1_TO_LINE3 = 0;  // Y junction routing Line 1 → Line 3
const int ROUTE_LINE2_TO_LINE3 = 1;  // Y junction routing Line 2 → Line 3

// Analog threshold for LED sensing (410 ≈ 2.0V with 5V reference)
const int ANALOG_THRESHOLD = 410;
const int ANALOG_HYSTERESIS = 20;  // Deadband around threshold to suppress chatter

const unsigned long DEBOUNCE_DELAY = 50;          // Debounce delay (ms)
const unsigned long SENSOR_SAMPLE_INTERVAL = 10;  // Minimum interval between reads (ms)

// ============================================================================
// DATA STRUCTURES
// ============================================================================

enum PointType {
  POINT_COACTING,
  POINT_Y_BRANCH
};

struct RelayPair {
  int line1;
  int line2;
};

struct AnalogSensor {
  int pin;
  int threshold;
  int hysteresis;
  int lastAnalog;             // Last analog reading captured on change
  unsigned long lastSample;   // Last time the pin was sampled
  unsigned long lastChanged;  // Last time the instantaneous state flipped
};

typedef struct {
  const char* label;          // Friendly name for logging
  PointType type;             // Coacting crossover or Y junction
  RelayPair relays;           // Relay outputs controlling the point
  AnalogSensor sensor;        // Sensor configuration and timing
  int currentState;           // Debounced logical state (CROSSOVER_* or ROUTE_*)
  int instantaneousState;     // Most recent instantaneous interpretation
  int reportedState;          // State most recently reported to Serial
} PointController;

int interpretState(const PointController& point, int analogValue, int priorState);
void initializePoint(PointController& point);
void updatePoint(PointController& point, unsigned long now);
void applyPointLogic(PointController& point);
const char* describeState(const PointController& point, int state);

PointController pointControllers[] = {
  {
    "Coacting Crossover",
    POINT_COACTING,
    {RELAY_LINE1_RIGHT_TO_LEFT, RELAY_LINE2_LEFT_TO_RIGHT},
    {CROSSOVER_SENSOR_PIN, ANALOG_THRESHOLD, ANALOG_HYSTERESIS, 0, 0, 0},
    CROSSOVER_ACTIVE,
    CROSSOVER_ACTIVE,
    -1
  },
  {
    "North Y Junction",
    POINT_Y_BRANCH,
    {RELAY_Y1_LINE1, RELAY_Y1_LINE2},
    {Y1_SENSOR_PIN, ANALOG_THRESHOLD, ANALOG_HYSTERESIS, 0, 0, 0},
    ROUTE_LINE1_TO_LINE3,
    ROUTE_LINE1_TO_LINE3,
    -1
  },
  {
    "South Y Junction",
    POINT_Y_BRANCH,
    {RELAY_Y2_LINE1, RELAY_Y2_LINE2},
    {Y2_SENSOR_PIN, ANALOG_THRESHOLD, ANALOG_HYSTERESIS, 0, 0, 0},
    ROUTE_LINE1_TO_LINE3,
    ROUTE_LINE1_TO_LINE3,
    -1
  }
};

const size_t POINT_COUNT = sizeof(pointControllers) / sizeof(pointControllers[0]);

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(9600);
  Serial.println("Railway Control System Initializing...");

  for (size_t i = 0; i < POINT_COUNT; ++i) {
    pinMode(pointControllers[i].relays.line1, OUTPUT);
    pinMode(pointControllers[i].relays.line2, OUTPUT);

    // Safe default: remove power from both approaches until state determined
    digitalWrite(pointControllers[i].relays.line1, TRACK_POWER_OFF);
    digitalWrite(pointControllers[i].relays.line2, TRACK_POWER_OFF);

    initializePoint(pointControllers[i]);
  }

  Serial.println("System Ready!");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  unsigned long now = millis();

  for (size_t i = 0; i < POINT_COUNT; ++i) {
    updatePoint(pointControllers[i], now);
  }
}

// ============================================================================
// FUNCTIONS
// ============================================================================

void initializePoint(PointController& point) {
  unsigned long now = millis();

  point.sensor.lastSample = now - SENSOR_SAMPLE_INTERVAL;  // allow immediate sampling
  point.sensor.lastChanged = now;

  int analogValue = analogRead(point.sensor.pin);
  point.sensor.lastAnalog = analogValue;

  point.currentState = interpretState(point, analogValue, point.currentState);
  point.instantaneousState = point.currentState;
  point.reportedState = -1;  // force initial log

  applyPointLogic(point);
}

void updatePoint(PointController& point, unsigned long now) {
  if ((now - point.sensor.lastSample) < SENSOR_SAMPLE_INTERVAL) {
    return;  // Respect sampling interval for non-blocking behaviour
  }

  point.sensor.lastSample = now;

  int analogValue = analogRead(point.sensor.pin);
  int reading = interpretState(point, analogValue, point.currentState);

  if (reading != point.instantaneousState) {
    point.instantaneousState = reading;
    point.sensor.lastChanged = now;
  }

  if ((now - point.sensor.lastChanged) >= DEBOUNCE_DELAY && reading != point.currentState) {
    point.currentState = reading;
    point.sensor.lastAnalog = analogValue;
    point.reportedState = -1;  // force status log on change
    applyPointLogic(point);
  }
}

int interpretState(const PointController& point, int analogValue, int priorState) {
  int lowerBound = constrain(point.sensor.threshold - point.sensor.hysteresis, 0, 1023);
  int upperBound = constrain(point.sensor.threshold + point.sensor.hysteresis, 0, 1023);

  if (point.type == POINT_COACTING) {
    if (priorState == CROSSOVER_ACTIVE) {
      return (analogValue > upperBound) ? CROSSOVER_INACTIVE : CROSSOVER_ACTIVE;
    }
    return (analogValue < lowerBound) ? CROSSOVER_ACTIVE : CROSSOVER_INACTIVE;
  }

  // POINT_Y_BRANCH logic with hysteresis
  if (priorState == ROUTE_LINE1_TO_LINE3) {
    return (analogValue > upperBound) ? ROUTE_LINE2_TO_LINE3 : ROUTE_LINE1_TO_LINE3;
  }
  return (analogValue < lowerBound) ? ROUTE_LINE1_TO_LINE3 : ROUTE_LINE2_TO_LINE3;
}

void applyPointLogic(PointController& point) {
  if (point.type == POINT_COACTING) {
    if (point.currentState == CROSSOVER_ACTIVE) {
      digitalWrite(point.relays.line1, TRACK_POWER_OFF);
      digitalWrite(point.relays.line2, TRACK_POWER_OFF);
    } else {
      digitalWrite(point.relays.line1, TRACK_POWER_ON);
      digitalWrite(point.relays.line2, TRACK_POWER_ON);
    }
  } else {  // POINT_Y_BRANCH
    if (point.currentState == ROUTE_LINE1_TO_LINE3) {
      digitalWrite(point.relays.line1, TRACK_POWER_ON);
      digitalWrite(point.relays.line2, TRACK_POWER_OFF);
    } else {
      digitalWrite(point.relays.line1, TRACK_POWER_OFF);
      digitalWrite(point.relays.line2, TRACK_POWER_ON);
    }
  }

  if (point.reportedState != point.currentState) {
    Serial.print(point.label);
    Serial.print(" -> ");
    Serial.println(describeState(point, point.currentState));

    Serial.print("  Analog reading: ");
    Serial.print(point.sensor.lastAnalog);
    Serial.print(" (threshold: ");
    Serial.print(point.sensor.threshold);
    Serial.println(")");

    point.reportedState = point.currentState;
  }
}

const char* describeState(const PointController& point, int state) {
  if (point.type == POINT_COACTING) {
    return (state == CROSSOVER_ACTIVE)
               ? "ACTIVE (both blocked)"
               : "INACTIVE (both free)";
  }

  return (state == ROUTE_LINE1_TO_LINE3)
             ? "Line 1 → Line 3 (Line 2 blocked)"
             : "Line 2 → Line 3 (Line 1 blocked)";
}
