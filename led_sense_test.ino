/*
 * LED Sense Test - Analog vs Digital Reading Comparison
 *
 * This test reads the LED voltage from a Tortoise motor using both
 * analog and digital methods to determine which is more reliable.
 *
 * Expected voltages:
 * - Red LED: ~1.9V (Crossover ACTIVE)
 * - Green LED: ~2.1V (Crossover INACTIVE)
 *
 * Arduino thresholds:
 * - Digital: ~2.5V (anything above reads HIGH, below reads LOW)
 * - Analog: 0-1023 (0 = 0V, 1023 = 5V reference)
 */

// ============================================================================
// PIN DEFINITIONS
// ============================================================================

const int LED_SENSE_PIN = 2;  // Pin to test (can be used for both digital and analog on most pins)

// Note: For analog reading, we'll use A2 which corresponds to digital pin 2 on some boards
// Adjust this based on your Arduino model
const int LED_ANALOG_PIN = A2;  // Analog input pin (change to A0-A5 as needed for your wiring)

// ============================================================================
// CONSTANTS
// ============================================================================

const unsigned long SAMPLE_INTERVAL = 100;  // Sample every 100ms
const unsigned long REPORT_INTERVAL = 1000; // Report statistics every 1 second

// Voltage calculation (5V reference, 10-bit ADC)
const float VOLTAGE_REFERENCE = 5.0;
const int ADC_RESOLUTION = 1024;

// Threshold for analog reading (midpoint between 1.9V and 2.1V = 2.0V)
const int ANALOG_THRESHOLD = (2.0 / VOLTAGE_REFERENCE) * ADC_RESOLUTION;  // ~410

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
  Serial.println("LED Sense Test - Analog vs Digital Comparison");
  Serial.println("=======================================================");
  Serial.println();

  // Configure pins
  pinMode(LED_SENSE_PIN, INPUT);  // For digital reading (no pullup)
  pinMode(LED_ANALOG_PIN, INPUT); // For analog reading

  Serial.println("Pin Configuration:");
  Serial.println("  Digital Pin: " + String(LED_SENSE_PIN));
  Serial.println("  Analog Pin:  A" + String(LED_ANALOG_PIN - A0));
  Serial.println();

  Serial.println("Analog Threshold: " + String(ANALOG_THRESHOLD) + " (~" + String((ANALOG_THRESHOLD * VOLTAGE_REFERENCE) / ADC_RESOLUTION, 2) + "V)");
  Serial.println("Digital Threshold: ~2.5V (built-in)");
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
 */
void takeSample() {
  // Read digital
  digitalReading = digitalRead(LED_SENSE_PIN);

  // Read analog
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
