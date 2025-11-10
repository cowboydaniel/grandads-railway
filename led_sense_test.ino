/*
 * LED Sense Test - Analog vs Digital Reading Comparison
 *
 * This test reads a bipolar LED from a Tortoise motor using both
 * analog and digital methods to determine which is more reliable.
 *
 * BIPOLAR LED Operation:
 * - When current flows one direction -> GREEN LED on -> Pin reads HIGH
 * - When current flows other direction -> RED LED on -> Pin reads LOW
 *
 * Testing:
 * - Digital: Direct HIGH/LOW reading
 * - Analog: Numeric value (helps detect noise/fluctuations)
 *
 * This test determines which method provides more stable/reliable readings.
 */

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

const int LED_SENSE_PIN = 2;  // Digital pin to read bipolar LED state

// For analog reading comparison (use appropriate analog pin based on your wiring)
const int LED_ANALOG_PIN = A2;  // Change to match your actual wiring (A0-A5)

// ============================================================================
// CONSTANTS
// ============================================================================

const unsigned long SAMPLE_INTERVAL = 100;  // Sample every 100ms
const unsigned long REPORT_INTERVAL = 1000; // Report statistics every 1 second

// Voltage calculation (5V reference, 10-bit ADC)
const float VOLTAGE_REFERENCE = 5.0;
const int ADC_RESOLUTION = 1024;

// Threshold for analog reading (set to 2V to match LED characteristics)
const int ANALOG_THRESHOLD = 410;  // 2.0V = (2.0/5.0) * 1024 ≈ 410

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

unsigned long lastSampleTime = 0;
unsigned long lastReportTime = 0;

// Current readings
int digitalReading = 0;
int analogReading = 0;
float voltage = 0.0;
int analogState = 0;  // HIGH or LOW based on threshold

// Statistics
unsigned long totalSamples = 0;
unsigned long digitalHighCount = 0;
unsigned long digitalLowCount = 0;
unsigned long analogHighCount = 0;
unsigned long analogLowCount = 0;
unsigned long digitalTransitions = 0;
unsigned long analogTransitions = 0;

int lastDigitalReading = 0;
int lastAnalogState = 0;

// Running statistics for analog readings
long analogSum = 0;
int analogMin = 1023;
int analogMax = 0;

// ============================================================================
// SETUP
// ============================================================================

void setup() {
  Serial.begin(9600);

  Serial.println("=======================================================");
  Serial.println("Bipolar LED Sense Test - Analog vs Digital");
  Serial.println("=======================================================");
  Serial.println();
  Serial.println("Testing bipolar LED from Tortoise motor:");
  Serial.println("  HIGH = GREEN LED (one polarity)");
  Serial.println("  LOW  = RED LED   (opposite polarity)");
  Serial.println();

  // Configure pins
  pinMode(LED_SENSE_PIN, INPUT);  // For digital reading (no pullup needed)
  pinMode(LED_ANALOG_PIN, INPUT); // For analog reading

  Serial.println("Pin Configuration:");
  Serial.println("  Digital Pin: " + String(LED_SENSE_PIN));
  Serial.println("  Analog Pin:  A" + String(LED_ANALOG_PIN - A0));
  Serial.println();

  Serial.println("Thresholds:");
  Serial.println("  Digital: ~2.5V (built-in Arduino threshold)");
  Serial.println("  Analog:  " + String(ANALOG_THRESHOLD) + " (~" + String((ANALOG_THRESHOLD * VOLTAGE_REFERENCE) / ADC_RESOLUTION, 2) + "V)");
  Serial.println();

  Serial.println("Starting continuous monitoring...");
  Serial.println("-------------------------------------------------------");
  Serial.println();

  // Initial readings
  digitalReading = digitalRead(LED_SENSE_PIN);
  analogReading = analogRead(LED_ANALOG_PIN);
  lastDigitalReading = digitalReading;
  lastAnalogState = (analogReading >= ANALOG_THRESHOLD) ? HIGH : LOW;

  lastSampleTime = millis();
  lastReportTime = millis();
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
  unsigned long currentTime = millis();

  // Take samples at regular intervals
  if (currentTime - lastSampleTime >= SAMPLE_INTERVAL) {
    lastSampleTime = currentTime;
    takeSample();
  }

  // Print statistics at regular intervals
  if (currentTime - lastReportTime >= REPORT_INTERVAL) {
    lastReportTime = currentTime;
    printStatistics();
  }
}

// ============================================================================
// FUNCTIONS
// ============================================================================

/**
 * Take a sample using both digital and analog methods
 * Digital: Direct HIGH/LOW reading of bipolar LED polarity
 * Analog: Numeric reading to detect any noise or fluctuations
 */
void takeSample() {
  // Read digital (HIGH = green LED, LOW = red LED)
  digitalReading = digitalRead(LED_SENSE_PIN);

  // Read analog and convert to digital state for comparison
  analogReading = analogRead(LED_ANALOG_PIN);
  voltage = (analogReading * VOLTAGE_REFERENCE) / ADC_RESOLUTION;
  analogState = (analogReading >= ANALOG_THRESHOLD) ? HIGH : LOW;

  // Update statistics
  totalSamples++;

  // Count states
  if (digitalReading == HIGH) {
    digitalHighCount++;
  } else {
    digitalLowCount++;
  }

  if (analogState == HIGH) {
    analogHighCount++;
  } else {
    analogLowCount++;
  }

  // Track transitions
  if (digitalReading != lastDigitalReading) {
    digitalTransitions++;
    Serial.println(">>> DIGITAL TRANSITION: " + String(lastDigitalReading ? "HIGH" : "LOW") +
                   " -> " + String(digitalReading ? "HIGH" : "LOW"));
  }

  if (analogState != lastAnalogState) {
    analogTransitions++;
    Serial.println(">>> ANALOG TRANSITION:  " + String(lastAnalogState ? "HIGH" : "LOW") +
                   " -> " + String(analogState ? "HIGH" : "LOW") +
                   " (at " + String(voltage, 3) + "V)");
  }

  // Update analog statistics
  analogSum += analogReading;
  if (analogReading < analogMin) analogMin = analogReading;
  if (analogReading > analogMax) analogMax = analogReading;

  // Print current readings
  printCurrentReadings();

  // Update last readings
  lastDigitalReading = digitalReading;
  lastAnalogState = analogState;
}

/**
 * Print current readings in a compact format
 */
void printCurrentReadings() {
  Serial.print("Digital: ");
  Serial.print(digitalReading ? "HIGH" : "LOW ");
  Serial.print("  |  Analog: ");
  Serial.print(analogReading);
  Serial.print(" (");
  Serial.print(voltage, 3);
  Serial.print("V) -> ");
  Serial.print(analogState ? "HIGH" : "LOW ");

  // Show if they disagree
  if (digitalReading != analogState) {
    Serial.print("  *** DISAGREEMENT ***");
  }

  Serial.println();
}

/**
 * Print statistics summary
 */
void printStatistics() {
  Serial.println();
  Serial.println("================ STATISTICS (last " + String(REPORT_INTERVAL/1000) + "s) ================");
  Serial.println("Total Samples: " + String(totalSamples));
  Serial.println();

  // Digital stats
  Serial.println("DIGITAL Reading:");
  Serial.println("  HIGH: " + String(digitalHighCount) + " (" + String((digitalHighCount * 100.0) / totalSamples, 1) + "%)");
  Serial.println("  LOW:  " + String(digitalLowCount) + " (" + String((digitalLowCount * 100.0) / totalSamples, 1) + "%)");
  Serial.println("  Transitions: " + String(digitalTransitions));
  Serial.println();

  // Analog stats
  Serial.println("ANALOG Reading:");
  Serial.println("  HIGH: " + String(analogHighCount) + " (" + String((analogHighCount * 100.0) / totalSamples, 1) + "%)");
  Serial.println("  LOW:  " + String(analogLowCount) + " (" + String((analogLowCount * 100.0) / totalSamples, 1) + "%)");
  Serial.println("  Transitions: " + String(analogTransitions));

  if (totalSamples > 0) {
    float avgReading = (float)analogSum / totalSamples;
    float avgVoltage = (avgReading * VOLTAGE_REFERENCE) / ADC_RESOLUTION;
    float minVoltage = (analogMin * VOLTAGE_REFERENCE) / ADC_RESOLUTION;
    float maxVoltage = (analogMax * VOLTAGE_REFERENCE) / ADC_RESOLUTION;

    Serial.println("  Average: " + String(avgReading, 1) + " (" + String(avgVoltage, 3) + "V)");
    Serial.println("  Min:     " + String(analogMin) + " (" + String(minVoltage, 3) + "V)");
    Serial.println("  Max:     " + String(analogMax) + " (" + String(maxVoltage, 3) + "V)");
    Serial.println("  Range:   " + String(analogMax - analogMin) + " (" + String(maxVoltage - minVoltage, 3) + "V)");
  }

  Serial.println();

  // Disagreement analysis
  if (digitalTransitions != analogTransitions) {
    Serial.println(">>> TRANSITION MISMATCH: Digital=" + String(digitalTransitions) +
                   ", Analog=" + String(analogTransitions));
    Serial.println(">>> More reliable method appears to be: " +
                   String(digitalTransitions < analogTransitions ? "DIGITAL" : "ANALOG"));
  } else {
    Serial.println(">>> Both methods show same number of transitions");
  }

  Serial.println("=======================================================");
  Serial.println();

  // Reset interval statistics (keep running totals)
}
