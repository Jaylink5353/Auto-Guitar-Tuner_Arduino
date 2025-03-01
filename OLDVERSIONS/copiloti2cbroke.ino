#include <Adafruit_GFX.h>
#include <Servo.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <arduinoFFT.h>


#define SAMPLES 64
#define SAMPLING_FREQUENCY 2048

double vReal[SAMPLES]; // Real values array
double vImag[SAMPLES]; // Imaginary values array

ArduinoFFT<double> FFT(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);


// OLED Display Setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

int samplingPeriod;

// Button & Servo
#define BUTTON_PIN 6
Servo servo;

// Guitar Strings & Tunings
const char* tuningNames[] = {"Standard", "Half-Step Down", "Drop D"};
const double tuningFrequencies[][6] = {
  {82.41, 110.00, 146.83, 196.00, 246.94, 329.63},  // Standard E
  {77.78, 103.83, 138.59, 185.00, 233.08, 311.13},  // Half-Step Down
  {82.41, 110.00, 146.83, 196.00, 246.94, 329.63}   // Drop D (same except low E → D)
};
int tuningIndex = 0;
int stringIndex = 0;

void setup() { 
  Wire.begin();
  Wire.setClock(10000);
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, SCREEN_WIDTH, SCREEN_HEIGHT)) {
    Serial.println("SSD1306 allocation failed!");
    for(;;);  // Halt execution
  }



  // Button Setup
  pinMode(BUTTON_PIN, INPUT);
  
  // Servo Setup
  servo.attach(3);
  
  int samplingPeriod = round(1000000 * (1.0 / SAMPLING_FREQUENCY));

  displayTuning();
}

void loop() {
  // Handle Button Press for Changing Strings
  if (digitalRead(BUTTON_PIN) == HIGH) {
    delay(200);
    stringIndex = (stringIndex + 1) % 6;
    displayTuning();
    while (digitalRead(BUTTON_PIN) == HIGH);
  }

  // Perform FFT & Get Frequency
  double peak = getPeakFrequency();

  // Tune the Selected String
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

// Display Current Tuning & String
void displayTuning() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Tuning: ");
  display.println(tuningNames[tuningIndex]);
  
  const char* stringNames[] = {"Low E", "A", "D", "G", "B", "High E"};
  display.print("String: ");
  display.println(stringNames[stringIndex]);
  
  display.display();
}

// Show "Tuned" Messagea
void displayDone() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(20, 20);
  display.println("IN TUNE!");
  display.display();
  delay(1000);
  displayTuning();
}

