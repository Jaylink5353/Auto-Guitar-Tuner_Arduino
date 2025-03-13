#include <LiquidCrystal.h>
#include <Stepper.h>
#include <arduinoFFT.h>

#define SAMPLES 128
#define SAMPLING_FREQUENCY 2048
#define INPUT_PIN A5
#define TUNEVAR -5

#define STEPS_PER_REV 2048

double vReal[SAMPLES]; // Real values array
double vImag[SAMPLES]; // Imaginary values array

arduinoFFT FFT = arduinoFFT();  // Correctly initialize FFT object

// LCD Setup: (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

// Stepper Motor Setup         1N4,1N2,1N3,1N1
Stepper stepper(STEPS_PER_REV, 2, 3, 4, 5); 

// Guitar Tunings
const char tuningNames[][17] = {"Standard", "Half-Step Down", "Drop D"};
const double tuningFrequencies[][6] = {
  {82, 110.00, 146.83, 196.00, 246.94, 329.63},  // Standard E
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
  lcd.setCursor(1, 0);
  lcd.print("Jaymes Goddard");

  // Button & Stepper Setup
  pinMode(INPUT_PIN, INPUT);
  pinMode(A0, INPUT);
  
  // Initialize stepper motor
  stepper.setSpeed(15);  // Set stepper motor speed
  
  samplingPeriod = round(1000000 * (1.0 / SAMPLING_FREQUENCY));

  delay(1500);
  displayTuning();
  stepper.step(300);
}

void loop() {
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
  Serial.print(result);
  // Run the tuning logic
  double peak = getPeakFrequency();
  Serial.print(peak);
  tuneString(peak, tuningFrequencies[tuningIndex][stringIndex]);
}

// FFT Processing to Get Peak Frequency
double getPeakFrequency() {
    for(int i = 0; i < SAMPLES; i++){
    float microSeconds = micros();    //Returns the number of microseconds since the Arduino board began running the current script. 

    vReal[i] = analogRead(0); //Reads the value from analog pin 0 (the sound sensor's analog output), quantize it and save it as a real term.
    vImag[i] = 0; //Makes imaginary term 0 always

    /*remaining wait time between samples if necessary*/
    while(micros() < (microSeconds + samplingPeriod))
    {
      //do nothing
    }
  }

  /*Perform FFT on samples*/
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

  /*Find peak frequency and print peak*/
  double peakF = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY) + TUNEVAR;  // peak is the most dominant frequency heard
  Serial.print("     Peak frequency:    ");
  Serial.println(peakF);
  return peakF;
}

// Tuning Logic
void tuneString(double peak, double targetFreq) {
  double tolerance = 2.0;
  if (analogRead(A0) < 805) { 
    stepper.step(0);  
  } else if (peak < targetFreq - tolerance) {
    stepper.step(50);  
    delay(700);
    stepper.step(-50);
  } else if (peak > targetFreq + tolerance) {
    stepper.step(-50);
    delay(700);
    stepper.step(50);
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
