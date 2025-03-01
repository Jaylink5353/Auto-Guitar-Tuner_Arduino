#include <LiquidCrystal.h>
#include <Servo.h>
#include <arduinoFFT.h>

#define SAMPLES 128
#define SAMPLING_FREQUENCY 2048

double vReal[SAMPLES]; // Real values array
double vImag[SAMPLES]; // Imaginary values array

ArduinoFFT<double> FFT(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

// LCD Setup: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Button & Servo
#define BUTTON_PIN 6
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
  lcd.print("Guitar Tuner");

  // Button & Servo Setup
  pinMode(BUTTON_PIN, INPUT);
  servo.attach(3);
  
  samplingPeriod = round(1000000 * (1.0 / SAMPLING_FREQUENCY));

  delay(1500);
  displayTuning();
}

void loop() {
  static bool lastButtonState = HIGH;
  static unsigned long buttonPressTime = 0;
  
  bool buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW && lastButtonState == HIGH) {  // Button just pressed
    buttonPressTime = millis();  // Record press time
  }

  if (buttonState == HIGH && lastButtonState == LOW) {  // Button just released
    unsigned long pressDuration = millis() - buttonPressTime;
    
    if (pressDuration >= LONG_PRESS_TIME) {
      // Long press: Change tuning
      tuningIndex = (tuningIndex + 1) % 3;  
      Serial.println("Tuning Changed: " + String(tuningNames[tuningIndex]));
    } else {
      // Short press: Change string
      stringIndex = (stringIndex + 1) % 6;
      Serial.println("String Changed: " + String(stringIndex));
    }

    displayTuning();  // Update screen
  }

  lastButtonState = buttonState;

  // Run the tuning logic
  double peak = getPeakFrequency();
  tuneString(peak, tuningFrequencies[tuningIndex][stringIndex]);

  double peak = getPeakFrequency();
  tuneString(peak, tuningFrequencies[tuningIndex][stringIndex]);
}

// FFT Processing to Get Peak Frequency
double getPeakFrequency() {
  for (int i = 0; i < SAMPLES; i++) {
    int microSeconds = micros();
    vReal[i] = analogRead(A0);
    vImag[i] = 0;
    while (micros() < (microSeconds + samplingPeriod));
  }
  FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.complexToMagnitude(vReal, vImag, SAMPLES);

  return FFT.majorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);
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
