/*
 * Railway Control System - Coacting Crossover and Dual Y Junctions
 *
 * Controls three point motors from a single Arduino using LED feedback from
 * Tortoise motors. One coacting crossover blocks both lines when active, while
 * two independent Y junctions each select between Line 1 or Line 2 feeding
 * Line 3. For each Y junction the non-selected approach is killed by removing
 * track power. All logic is completely non-blocking and driven by sensor
 * sampling with millisecond timing.
 *
 * Y JUNCTION LOGIC (applies to both junctions):
 *   - Route Line 1 → Line 3  ⇒  Line 1 powered, Line 2 killed
 *   - Route Line 2 → Line 3  ⇒  Line 2 powered, Line 1 killed
 *
 * COACTING CROSSOVER LOGIC:
 *   - ACTIVE   ⇒  Both lines blocked
 *   - INACTIVE ⇒  Both lines free
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
const int TRACK_POWER_OFF = LOW;     // Relay ON  (power removed from track)

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

typedef struct {
  const char* label;        // Friendly name for logging
  PointType type;           // Coacting crossover or Y junction
  int sensorPin;            // Analog sensor input
  int relayLine1;           // Relay controlling Line 1 feed
  int relayLine2;           // Relay controlling Line 2 feed
  int analogThreshold;      // Threshold separating LED polarity states
  int analogHysteresis;     // Hysteresis band to prevent chatter
  int currentState;         // Debounced state (CROSSOVER_* or ROUTE_*)
  int lastReading;          // Most recent instantaneous reading
  unsigned long lastDebounceTime;
  unsigned long lastSampleTime;
  int lastAnalogValue;      // Analog value captured when state last changed
  int lastStatus;           // Tracks last status reported to Serial
} PointControl;

const char* describeState(const PointControl& point, int state);
void initializePoint(PointControl& point);
void updatePoint(PointControl& point, unsigned long now);
int interpretState(const PointControl& point, int analogValue, int priorState);
void applyPointLogic(PointControl& point);

PointControl pointControls[] = {
  {
    "Coacting Crossover",
    POINT_COACTING,
    CROSSOVER_SENSOR_PIN,
    RELAY_LINE1_RIGHT_TO_LEFT,
    RELAY_LINE2_LEFT_TO_RIGHT,
    ANALOG_THRESHOLD,
    ANALOG_HYSTERESIS,
    CROSSOVER_ACTIVE,
    CROSSOVER_ACTIVE,
    0,
    0,
    0,
    -1
  },
  {
    "North Y Junction",
    POINT_Y_BRANCH,
    Y1_SENSOR_PIN,
    RELAY_Y1_LINE1,
    RELAY_Y1_LINE2,
    ANALOG_THRESHOLD,
    ANALOG_HYSTERESIS,
    ROUTE_LINE1_TO_LINE3,
    ROUTE_LINE1_TO_LINE3,
    0,
    0,
    0,
    -1
  },
  {
    "South Y Junction",
    POINT_Y_BRANCH,
    Y2_SENSOR_PIN,
    RELAY_Y2_LINE1,
    RELAY_Y2_LINE2,
    ANALOG_THRESHOLD,
    ANALOG_HYSTERESIS,
    ROUTE_LINE1_TO_LINE3,
    ROUTE_LINE1_TO_LINE3,
    0,
    0,
    0,
    -1
  }
};

const size_t POINT_COUNT = sizeof(pointControls) / sizeof(pointControls[0]);

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(9600);
  Serial.println("Railway Control System Initializing...");

  for (size_t i = 0; i < POINT_COUNT; ++i) {
    pinMode(pointControls[i].relayLine1, OUTPUT);
    pinMode(pointControls[i].relayLine2, OUTPUT);

    // Safe default: remove power from both approaches until state determined
    digitalWrite(pointControls[i].relayLine1, TRACK_POWER_OFF);
    digitalWrite(pointControls[i].relayLine2, TRACK_POWER_OFF);

    initializePoint(pointControls[i]);
  }

  Serial.println("System Ready!");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  unsigned long now = millis();

  for (size_t i = 0; i < POINT_COUNT; ++i) {
    updatePoint(pointControls[i], now);
  }
}

// ============================================================================
// FUNCTIONS
// ============================================================================

void initializePoint(PointControl& point) {
  unsigned long now = millis();

  point.lastDebounceTime = now;
  point.lastSampleTime = now - SENSOR_SAMPLE_INTERVAL;  // allow immediate sampling

  int analogValue = analogRead(point.sensorPin);
  point.lastAnalogValue = analogValue;
  point.currentState = interpretState(point, analogValue, point.currentState);
  point.lastReading = point.currentState;
  point.lastStatus = -1;  // force log on first apply

  applyPointLogic(point);
}

void updatePoint(PointControl& point, unsigned long now) {
  if ((now - point.lastSampleTime) < SENSOR_SAMPLE_INTERVAL) {
    return;  // Respect sampling interval for non-blocking behaviour
  }

  point.lastSampleTime = now;

  int analogValue = analogRead(point.sensorPin);
  int reading = interpretState(point, analogValue, point.currentState);

  if (reading != point.lastReading) {
    point.lastDebounceTime = now;
  }

  point.lastReading = reading;

  if ((now - point.lastDebounceTime) >= DEBOUNCE_DELAY && point.currentState != reading) {
    point.currentState = reading;
    point.lastAnalogValue = analogValue;
    point.lastStatus = -1;  // force status log on change
  }

  applyPointLogic(point);
}

int interpretState(const PointControl& point, int analogValue, int priorState) {
  int lowerBound = constrain(point.analogThreshold - point.analogHysteresis, 0, 1023);
  int upperBound = constrain(point.analogThreshold + point.analogHysteresis, 0, 1023);

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

void applyPointLogic(PointControl& point) {
  if (point.type == POINT_COACTING) {
    if (point.currentState == CROSSOVER_ACTIVE) {
      digitalWrite(point.relayLine1, TRACK_POWER_OFF);
      digitalWrite(point.relayLine2, TRACK_POWER_OFF);
    } else {
      digitalWrite(point.relayLine1, TRACK_POWER_ON);
      digitalWrite(point.relayLine2, TRACK_POWER_ON);
    }
  } else {  // POINT_Y_BRANCH
    if (point.currentState == ROUTE_LINE1_TO_LINE3) {
      digitalWrite(point.relayLine1, TRACK_POWER_ON);
      digitalWrite(point.relayLine2, TRACK_POWER_OFF);
    } else {
      digitalWrite(point.relayLine1, TRACK_POWER_OFF);
      digitalWrite(point.relayLine2, TRACK_POWER_ON);
    }
  }

  if (point.lastStatus != point.currentState) {
    Serial.print(point.label);
    Serial.print(" -> ");
    Serial.println(describeState(point, point.currentState));

    Serial.print("  Analog reading: ");
    Serial.print(point.lastAnalogValue);
    Serial.print(" (threshold: ");
    Serial.print(point.analogThreshold);
    Serial.println(")");

    point.lastStatus = point.currentState;
  }
}

const char* describeState(const PointControl& point, int state) {
  if (point.type == POINT_COACTING) {
    return (state == CROSSOVER_ACTIVE)
               ? "ACTIVE (both blocked)"
               : "INACTIVE (both free)";
  }

  return (state == ROUTE_LINE1_TO_LINE3)
             ? "Line 1 → Line 3 (Line 2 blocked)"
             : "Line 2 → Line 3 (Line 1 blocked)";
}
