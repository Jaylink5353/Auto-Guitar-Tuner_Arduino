#include <LiquidCrystal.h>
#include <Stepper.h>

#define SAMPLES 128
#define SAMPLING_FREQUENCY 2048
#define INPUT_PIN A5
#define BUTTON_PIN A0  // Use a separate analog pin for the button

float vReal[SAMPLES]; // Real values array

// LCD Setup: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Stepper motor setup
const int stepsPerRevolution = 2048;  // Change this value to match your stepper motor
Stepper stepper(stepsPerRevolution, 2, 3, 4, 5);  // Pins connected to the stepper motor

// Guitar Tunings
const char tuningNames[][17] = {"Standard", "Half-Step Down", "Drop D"};
const double tuningFrequencies[][6] = {
  {82.41, 110.00, 146.83, 196.00, 246.94, 329.63},  // Standard E
  {77.78, 103.83, 138.59, 185.00, 233.08, 311.13},  // Half-Step Down
  {82.41, 110.00, 146.83, 196.00, 246.94, 329.63}   // Drop D (Low E → D)
};

int tuningIndex = 0;
int stringIndex = 0;
int samplingPeriod;

void setup() {
  Serial.begin(9600);

  // Initialize LCD
  lcd.begin(16, 2);  
  lcd.print("Guitar Tuner By:");
  lcd.setCursor(1,0);
  lcd.print("Jaymes Goddard");

  // Button & Stepper Setup
  pinMode(INPUT_PIN, INPUT);
  stepper.setSpeed(15);  // Set the speed of the stepper motor
  
  samplingPeriod = round(1000000 * (1.0 / SAMPLING_FREQUENCY));

  delay(1500);
  displayTuning();
}

void loop() {
  static unsigned long lastSampleTime = 0;
  static int sampleIndex = 0;

  // Check buttons frequently
  int result = readAnalogButton();
  if (result == 1) {
    // Tuning button
    tuningIndex = (tuningIndex + 1) % 3; 
    displayTuning();
  }
  if (result == 2) {
    // String button
    stringIndex = (stringIndex + 1) % 6;
    displayTuning();
  }

  // Sample data only if enough time has passed
  unsigned long currentTime = micros();
  if (currentTime - lastSampleTime >= samplingPeriod) {
    vReal[sampleIndex] = analogRead(A0);
    sampleIndex++;
    lastSampleTime = currentTime;

    if (sampleIndex >= SAMPLES) {
      sampleIndex = 0;
      double peak = getPeakFrequency();
      Serial.print(peak);
      tuneString(peak, tuningFrequencies[tuningIndex][stringIndex]);
    }
  }
}

// Zero-Crossing Detection to Get Peak Frequency
double getPeakFrequency() {
  Serial.println("Starting getPeakFrequency");
  Serial.print("Available memory before loop: ");
  Serial.println(availableMemory());

  // Zero-Crossing Detection
  int zeroCrossings = 0;
  for (int i = 1; i < SAMPLES; i++) {
    if ((vReal[i - 1] < 512 && vReal[i] >= 512) || (vReal[i - 1] >= 512 && vReal[i] < 512)) {
      zeroCrossings++;
    }
  }
  Serial.print("Zero crossings: ");
  Serial.println(zeroCrossings);

  double peakFrequency = (zeroCrossings / 2.0) * (SAMPLING_FREQUENCY / SAMPLES);
  Serial.print("Peak frequency: ");
  Serial.println(peakFrequency);

  Serial.println("Finished getPeakFrequency");
  return peakFrequency;
}

// Tuning Logic
void tuneString(double peak, double targetFreq) {
  double tolerance = 2.0;
  if (analogRead(A0) < 805) { 
    stepper.step(0);  
  } else if (peak < targetFreq - tolerance) {
    stepper.step(stepsPerRevolution);  // Adjust the steps as needed
  } else if (peak > targetFreq + tolerance) {
    stepper.step(-stepsPerRevolution);  // Adjust the steps as needed
  } else {
    displayDone();
  }
}

// Display Current Tuning & String on LCD
void displayTuning() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Tuning: ");
  lcd.print(tuningNames[tuningIndex]);

  const char stringNames[][7] = {"Low E", "A", "D", "G", "B", "High E"};
  lcd.setCursor(0, 1);
  lcd.print("String: ");
  lcd.print(stringNames[stringIndex]);
}

// Show "Tuned" Message
void displayDone() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("IN TUNE!");
  delay(1000);
  displayTuning();
}

// Read Buttons
int readAnalogButton() {
  int button = analogRead(INPUT_PIN);
  if (button > 921) return 0;
  if (button < 256) return 1;
  if (button < 598) return 2;
}

int availableMemory() {
    // Use 1024 with ATmega168
    int size = 2048;
    byte *buf;
    while ((buf = (byte *) malloc(--size)) == NULL);
        free(buf);
    return size;
}