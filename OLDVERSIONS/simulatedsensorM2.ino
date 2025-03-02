#include <LiquidCrystal.h>
#include <Servo.h>

#define SAMPLES 8  // Further reduce the number of samples
#define SAMPLING_FREQUENCY 2048
#define INPUT_PIN A5

float vReal[SAMPLES]; // Real values array

// LCD Setup: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

Servo servo;

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

  // Button & Servo Setup
  pinMode(INPUT_PIN, INPUT);
  servo.attach(3);
  
  samplingPeriod = round(1000000 * (1.0 / SAMPLING_FREQUENCY));

  delay(1500);
  displayTuning();
}

void loop() {
  int result = readAnalogButton();
  if (result == 1){
    //tuningbutton
    tuningIndex = (tuningIndex + 1) % 3; 
    displayTuning();
  }
  if (result == 2){
    //string button
    stringIndex = (stringIndex + 1) % 6;
    displayTuning();
  }
  Serial.print(result);
  // Run the tuning logic
  double peak = getPeakFrequency();
  Serial.print(peak);
  tuneString(peak, tuningFrequencies[tuningIndex][stringIndex]);
}

// Zero-Crossing Detection to Get Peak Frequency
double getPeakFrequency() {
  Serial.println("Starting getPeakFrequency");
  Serial.print("Available memory before loop: ");
  Serial.println(availableMemory());

  for (int i = 0; i < SAMPLES; i++) {
    vReal[i] = 512 + 100 * sin(2 * PI * i / SAMPLES);  // Simulated sinusoidal data
    Serial.print("vReal[");
    Serial.print(i);
    Serial.print("]: ");
    Serial.println(vReal[i]);

    delayMicroseconds(samplingPeriod);  // Simplified delay with delayMicroseconds
  }
  Serial.println("Finished for loop");

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
    servo.write(90);  
  } else if (peak < targetFreq - tolerance) {
    servo.write(60);  
    delay(700);
    servo.write(90);
  } else if (peak > targetFreq + tolerance) {
    servo.write(120);
    delay(700);
    servo.write(90);
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
