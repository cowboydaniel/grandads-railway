/*
 * Railway Crossover Control System
 *
 * Controls two railway lines with a crossover section using relays
 * to stop trains when points are against them.
 *
 * LINE CONFIGURATION:
 *
 * Line 1: __________________________________
 *                            \
 *                             \ crossover
 *                              \
 * Line 2: __________________________________
 *
 * LOGIC:
 * - Line 1, Left→Right: Always free (coacting points)
 * - Line 1, Right→Left: Stopped if points are against them
 * - Line 2, Left→Right: Stopped if points are against them
 * - Line 2, Right→Left: Always free (can go either way)
 */

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// Points position sensor (reads current points position)
// LOW = Points set for Line 1 (right to left)
// HIGH = Points set for Line 2 (left to right)
const int POINTS_SENSOR_PIN = 2;

// Relay control pins (Active LOW - relay energizes when pin is LOW)
const int RELAY_LINE1_RIGHT_TO_LEFT = 3;  // Controls power to Line 1, right approach
const int RELAY_LINE2_LEFT_TO_RIGHT = 4;  // Controls power to Line 2, left approach

// Optional: Points motor control (if using servo or motor to change points)
const int POINTS_MOTOR_PIN = 9;  // PWM pin for servo

// Optional: LED indicators for visual feedback
const int LED_LINE1_BLOCKED = 5;
const int LED_LINE2_BLOCKED = 6;
const int LED_POINTS_POSITION = 7;

// ============================================================================
// CONSTANTS
// ============================================================================

const int POINTS_FOR_LINE1 = LOW;   // Points set for Line 1 (right to left)
const int POINTS_FOR_LINE2 = HIGH;  // Points set for Line 2 (left to right)

const int TRACK_POWER_ON = HIGH;    // Relay OFF (power flows to track)
const int TRACK_POWER_OFF = LOW;    // Relay ON (power cut from track)

const unsigned long DEBOUNCE_DELAY = 50;  // Debounce delay in milliseconds

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

int currentPointsPosition = POINTS_FOR_LINE1;
int lastPointsReading = POINTS_FOR_LINE1;
unsigned long lastDebounceTime = 0;

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);
  Serial.println("Railway Crossover Control System Initializing...");

  // Configure input pins
  pinMode(POINTS_SENSOR_PIN, INPUT_PULLUP);

  // Configure relay output pins
  pinMode(RELAY_LINE1_RIGHT_TO_LEFT, OUTPUT);
  pinMode(RELAY_LINE2_LEFT_TO_RIGHT, OUTPUT);

  // Configure LED indicator pins
  pinMode(LED_LINE1_BLOCKED, OUTPUT);
  pinMode(LED_LINE2_BLOCKED, OUTPUT);
  pinMode(LED_POINTS_POSITION, OUTPUT);

  // Initialize all tracks to powered (safe default)
  digitalWrite(RELAY_LINE1_RIGHT_TO_LEFT, TRACK_POWER_ON);
  digitalWrite(RELAY_LINE2_LEFT_TO_RIGHT, TRACK_POWER_ON);

  // Initialize LEDs
  digitalWrite(LED_LINE1_BLOCKED, LOW);
  digitalWrite(LED_LINE2_BLOCKED, LOW);
  digitalWrite(LED_POINTS_POSITION, LOW);

  // Read initial points position
  currentPointsPosition = digitalRead(POINTS_SENSOR_PIN);

  Serial.println("System Ready!");
  Serial.println("Points Position: " + String(currentPointsPosition == POINTS_FOR_LINE1 ? "Line 1" : "Line 2"));
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Read points position with debouncing
  readPointsPosition();

  // Apply control logic based on points position
  controlTrackPower();

  // Small delay to prevent excessive loop iterations
  delay(10);
}

// ============================================================================
// FUNCTIONS
// ============================================================================

/**
 * Read points position with debouncing to prevent false triggers
 */
void readPointsPosition() {
  int reading = digitalRead(POINTS_SENSOR_PIN);

  // If reading changed, reset debounce timer
  if (reading != lastPointsReading) {
    lastDebounceTime = millis();
  }

  // If reading has been stable for debounce delay, accept it
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != currentPointsPosition) {
      currentPointsPosition = reading;

      // Log points position change
      Serial.println("Points changed to: " + String(currentPointsPosition == POINTS_FOR_LINE1 ? "Line 1 (R→L)" : "Line 2 (L→R)"));

      // Update points position indicator LED
      digitalWrite(LED_POINTS_POSITION, currentPointsPosition);
    }
  }

  lastPointsReading = reading;
}

/**
 * Control track power based on points position and traffic rules
 *
 * RULES:
 * - Line 1 L→R: Always powered (coacting points)
 * - Line 1 R→L: Powered only when points favor Line 1
 * - Line 2 L→R: Powered only when points favor Line 2
 * - Line 2 R→L: Always powered (free to go either way)
 */
void controlTrackPower() {
  if (currentPointsPosition == POINTS_FOR_LINE1) {
    // Points set for Line 1 (right to left traffic)

    // Line 1 R→L: POWER ON (points favor this direction)
    digitalWrite(RELAY_LINE1_RIGHT_TO_LEFT, TRACK_POWER_ON);
    digitalWrite(LED_LINE1_BLOCKED, LOW);

    // Line 2 L→R: POWER OFF (points against this direction)
    digitalWrite(RELAY_LINE2_LEFT_TO_RIGHT, TRACK_POWER_OFF);
    digitalWrite(LED_LINE2_BLOCKED, HIGH);

    if (Serial.available() == 0) {  // Only print once per change
      static int lastState = -1;
      if (lastState != 0) {
        Serial.println("Status: Line 1 R→L OPEN | Line 2 L→R BLOCKED");
        lastState = 0;
      }
    }
  }
  else {
    // Points set for Line 2 (left to right traffic)

    // Line 1 R→L: POWER OFF (points against this direction)
    digitalWrite(RELAY_LINE1_RIGHT_TO_LEFT, TRACK_POWER_OFF);
    digitalWrite(LED_LINE1_BLOCKED, HIGH);

    // Line 2 L→R: POWER ON (points favor this direction)
    digitalWrite(RELAY_LINE2_LEFT_TO_RIGHT, TRACK_POWER_ON);
    digitalWrite(LED_LINE2_BLOCKED, LOW);

    if (Serial.available() == 0) {  // Only print once per change
      static int lastState = -1;
      if (lastState != 1) {
        Serial.println("Status: Line 1 R→L BLOCKED | Line 2 L→R OPEN");
        lastState = 1;
      }
    }
  }
}

/**
 * Optional: Manual points control function
 * Call this to manually set points position
 */
void setPointsPosition(int position) {
  if (position == POINTS_FOR_LINE1 || position == POINTS_FOR_LINE2) {
    // If using a servo or motor to control points
    // Add servo control code here
    // servo.write(position == POINTS_FOR_LINE1 ? 0 : 90);

    Serial.println("Points manually set to: " + String(position == POINTS_FOR_LINE1 ? "Line 1" : "Line 2"));
  }
}

/**
 * Emergency stop function - cuts power to all controlled sections
 */
void emergencyStop() {
  digitalWrite(RELAY_LINE1_RIGHT_TO_LEFT, TRACK_POWER_OFF);
  digitalWrite(RELAY_LINE2_LEFT_TO_RIGHT, TRACK_POWER_OFF);
  digitalWrite(LED_LINE1_BLOCKED, HIGH);
  digitalWrite(LED_LINE2_BLOCKED, HIGH);

  Serial.println("EMERGENCY STOP ACTIVATED!");
}

/**
 * Resume normal operation after emergency stop
 */
void resumeOperation() {
  Serial.println("Resuming normal operation...");
  controlTrackPower();
}
