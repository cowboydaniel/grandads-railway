/*
 * Railway Crossover Control System - Coacting Point
 *
 * Controls two railway lines with a coacting crossover point using relays
 * to block both lines when crossover is active.
 *
 * LINE CONFIGURATION:
 *
 * Line 1: __________________________________
 *                            \
 *                             \ crossover
 *                              \
 * Line 2: __________________________________
 *
 * LOGIC (Coacting Point):
 * When crossover is ACTIVE (pin 2 LOW):
 *   - Line 1 Right→Left: BLOCKED
 *   - Line 2 Left→Right: BLOCKED
 * When crossover is INACTIVE (pin 2 HIGH):
 *   - Both lines: FREE
 */

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

// Crossover sensor - connected to Tortoise motor's bipolar LED
// Bipolar LED operation (current reverses polarity):
// - When RED LED on (one polarity) = Crossover ACTIVE (blocks both lines) - reads LOW
// - When GREEN LED on (opposite polarity) = Crossover INACTIVE (both lines free) - reads HIGH
// Pin directly senses the LED polarity/current direction
const int CROSSOVER_SENSOR_PIN = 2;

// Relay control pins (Active LOW - relay energizes when pin is LOW)
const int RELAY_LINE1_RIGHT_TO_LEFT = 3;  // Controls power to Line 1, right approach
const int RELAY_LINE2_LEFT_TO_RIGHT = 4;  // Controls power to Line 2, left approach

// ============================================================================
// CONSTANTS
// ============================================================================

const int CROSSOVER_ACTIVE = LOW;    // Crossover active (both lines blocked)
const int CROSSOVER_INACTIVE = HIGH; // Crossover inactive (both lines free)

const int TRACK_POWER_ON = HIGH;     // Relay OFF (power flows to track)
const int TRACK_POWER_OFF = LOW;     // Relay ON (power cut from track)

const unsigned long DEBOUNCE_DELAY = 50;  // Debounce delay in milliseconds

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

int currentCrossoverState = CROSSOVER_ACTIVE;
int lastCrossoverReading = CROSSOVER_ACTIVE;
unsigned long lastDebounceTime = 0;

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);
  Serial.println("Railway Crossover Control System - Coacting Point Initializing...");

  // Configure input pin (sensing LED voltage directly, no pullup needed)
  pinMode(CROSSOVER_SENSOR_PIN, INPUT);

  // Configure relay output pins
  pinMode(RELAY_LINE1_RIGHT_TO_LEFT, OUTPUT);
  pinMode(RELAY_LINE2_LEFT_TO_RIGHT, OUTPUT);

  // Initialize both tracks to blocked (safe default - crossover active)
  digitalWrite(RELAY_LINE1_RIGHT_TO_LEFT, TRACK_POWER_OFF);
  digitalWrite(RELAY_LINE2_LEFT_TO_RIGHT, TRACK_POWER_OFF);

  // Read initial crossover state
  currentCrossoverState = digitalRead(CROSSOVER_SENSOR_PIN);

  Serial.println("System Ready!");
  Serial.println("Crossover State: " + String(currentCrossoverState == CROSSOVER_ACTIVE ? "ACTIVE (both blocked)" : "INACTIVE (both free)"));
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  // Read crossover state with debouncing
  readCrossoverState();

  // Apply control logic based on crossover state
  controlTrackPower();

  // Small delay to prevent excessive loop iterations
  delay(10);
}

// ============================================================================
// FUNCTIONS
// ============================================================================

/**
 * Read crossover state with debouncing to prevent false triggers
 */
void readCrossoverState() {
  int reading = digitalRead(CROSSOVER_SENSOR_PIN);

  // If reading changed, reset debounce timer
  if (reading != lastCrossoverReading) {
    lastDebounceTime = millis();
  }

  // If reading has been stable for debounce delay, accept it
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != currentCrossoverState) {
      currentCrossoverState = reading;

      // Log crossover state change
      Serial.println("Crossover changed to: " + String(currentCrossoverState == CROSSOVER_ACTIVE ? "ACTIVE (both blocked)" : "INACTIVE (both free)"));
    }
  }

  lastCrossoverReading = reading;
}

/**
 * Control track power based on crossover state
 *
 * COACTING POINT LOGIC:
 * - When crossover ACTIVE (LOW): Both relays block both lines
 *   - Line 1 R→L: BLOCKED
 *   - Line 2 L→R: BLOCKED
 * - When crossover INACTIVE (HIGH): Both relays allow traffic
 *   - Line 1 R→L: FREE
 *   - Line 2 L→R: FREE
 */
void controlTrackPower() {
  if (currentCrossoverState == CROSSOVER_ACTIVE) {
    // Crossover is active - block both lines
    digitalWrite(RELAY_LINE1_RIGHT_TO_LEFT, TRACK_POWER_OFF);
    digitalWrite(RELAY_LINE2_LEFT_TO_RIGHT, TRACK_POWER_OFF);

    static int lastState = -1;
    if (lastState != 0) {
      Serial.println("Status: BOTH LINES BLOCKED");
      lastState = 0;
    }
  }
  else {
    // Crossover is inactive - both lines free
    digitalWrite(RELAY_LINE1_RIGHT_TO_LEFT, TRACK_POWER_ON);
    digitalWrite(RELAY_LINE2_LEFT_TO_RIGHT, TRACK_POWER_ON);

    static int lastState = -1;
    if (lastState != 1) {
      Serial.println("Status: BOTH LINES FREE");
      lastState = 1;
    }
  }
}

/**
 * Optional: Manual crossover control function
 * Call this to manually set crossover state
 */
void setCrossoverState(int state) {
  if (state == CROSSOVER_ACTIVE || state == CROSSOVER_INACTIVE) {
    // If using a servo or motor to control crossover
    // Add servo control code here
    // servo.write(state == CROSSOVER_ACTIVE ? 0 : 90);

    Serial.println("Crossover manually set to: " + String(state == CROSSOVER_ACTIVE ? "ACTIVE" : "INACTIVE"));
  }
}

/**
 * Emergency stop function - cuts power to all controlled sections
 */
void emergencyStop() {
  digitalWrite(RELAY_LINE1_RIGHT_TO_LEFT, TRACK_POWER_OFF);
  digitalWrite(RELAY_LINE2_LEFT_TO_RIGHT, TRACK_POWER_OFF);

  Serial.println("EMERGENCY STOP ACTIVATED!");
}

/**
 * Resume normal operation after emergency stop
 */
void resumeOperation() {
  Serial.println("Resuming normal operation...");
  controlTrackPower();
}
