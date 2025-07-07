#include <Adafruit_SSD1306.h>
#include <splash.h>

/* AUTOMATIC GUITAR TUNER

The following program takes in a sound wave from a guitar string being strum and uses it to determine if that string is
in tune. The sound wave is sampled and its peak frequency is calculated using FFT. If this frequency is outside the range of 
frequencies that are considered "in tune" for that pitch (within approx. 5 cents of the desired frequency for that string)
then a continuous rotation servo is activated to turn. The direction it turns depends on if the calculated frequency is too
high or too low. Once done tuning, the green LED is signaled to turn on. This process can be done for any of the six guitar 
strings. Pressing the select button will cycle through the red LEDs, which represent which string is being tuned.

*/

#include <Servo.h>
#include <arduinoFFT.h>



#define SAMPLES 128             //SAMPLES-pt FFT. Must be a base 2 number. Max 128 for Arduino Uno.
#define SAMPLING_FREQUENCY 4096//Ts = Based on Nyquist, must be 2 times the highest expected frequency.

arduinoFFT<float> FFT = arduinoFFT<float>(); 
 
unsigned int samplingPeriod;    // amount of time between samples
unsigned long microSeconds;     // keeps track of time elapsed
 
float vReal[SAMPLES]; //create vector of size SAMPLES to hold real values
float vImag[SAMPLES]; //create vector of size SAMPLES to hold imaginary values



bool inTune;          // flag that is true if the string is in tune, false otherwise
bool changeString;    // flag that is true if the user wants to change the string being tuned, false otherwise
int stringSelected;   // the pin that the string's LED is from

Servo servo;          

void setup()
{
  Serial.begin(9600);
  samplingPeriod = round(1000000*(1.0/SAMPLING_FREQUENCY)); //Period in microseconds 
  
  pinMode(6, INPUT);  // string select button

  pinMode(13, OUTPUT);  // DONE LED - green
  
  pinMode(12, OUTPUT);  // low E LED - red  
  pinMode(11, OUTPUT);  // B LED - red  
  pinMode(10, OUTPUT);  // G LED - red  
  pinMode(9, OUTPUT);   // D LED - red  
  pinMode(8, OUTPUT);   // A LED - red  
  pinMode(7, OUTPUT);   // high E LED - red  
  
  changeString = false;   // default to not wanting to change string
  stringSelected = 7;     // default to selecting low E to tune (pin 7)
  digitalWrite(stringSelected, HIGH);   // turn on red LED for the string selected
  
  servo.attach(3);  // attach servo to  Arduino pin 3
        // counterclockwise - tunes string higher (>90 servo setting)
        // clockwise - tunes string lower (<90 servo setting)
}

void loop()
{
  // while button is pressed, set flag to change string selected for tuning
  while (digitalRead(6) == HIGH) {
    changeString = true;        // now want to change string
  }

  // change the string selected and corresponding LED when button is released
  if (changeString) {         // if want to change string
    digitalWrite(stringSelected, LOW);  // turn off selected LED
    
    digitalWrite(13, LOW);      // turn off DONE LED
    
    // select new string to tune
    if (stringSelected == 12) {   
      stringSelected = 7;       // wrap around to beginning (7) if the last one (12)
    }
    else {
      stringSelected++;         // increment to change to next string if not the last one (12)
    }
    
    changeString = false;       // done changing string, deactivate flag
    digitalWrite(stringSelected, HIGH); // turn on new selected LED
  }
  
  // process sound (perform FFT) received through sound sensor
  
    /*Sample SAMPLES times*/
  for(int i=0; i<SAMPLES; i++)
  {
    microSeconds = micros();    //Returns the number of microseconds since the Arduino board began running the current script. 

    vReal[i] = analogRead(0); //Reads the value from analog pin 0 (the sound sensor's analog output), quantize it and save it as a real term.
    vImag[i] = 0; //Makes imaginary term 0 always

    /*remaining wait time between samples if necessary*/
    while(micros() < (microSeconds + samplingPeriod))
    {
      //do nothing
    }
  }

  /*Perform FFT on samples*/
  FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.complexToMagnitude(vReal, vImag, SAMPLES);

  /*Find peak frequency and print peak*/
  double peak = FFT.majorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);  // peak is the most dominant frequency heard
  Serial.print(peak);     //Print out the most dominant frequency.
  Serial.print("     ");
    Serial.println(analogRead(0));  // print out the sound intensity detected by the sound sensor

  // tuning a string
  
  /* Pitch ranges for each string were originally based off the actual frequency corresponding to each of the strings in a standard guitar tuning (EADGBe) +/- a ~3 Hz tolerance.
   *    E:  82.41 Hz
   *    A: 110.00 Hz
   *    D: 146.83 Hz
   *    G: 196.00 Hz
   *    B: 246.94 Hz 
   *    e: 246.94 Hz 
   *    
   * After doing some testing, it was found that for some of the strings, these values were not the ones being calculated through the FFT algorithm. 
   * It may have been returning frequencies of higher harmonics, so I chose the values that were most consistent with the results found in testing for final code. 
   * The strings most affected by this were the low E and D strings.
   * 
   * I also tested with a guitar tuner app to see what frequency was being calculated by the FFT when the string was in tune as indicated through the app.
   * Some of the values were a bit off from the actual frequencies listed above, so I adjust those values slightly accordingly based on the testing results. 
   * As a result, some of the ranges are 3-5 Hz different from the expected frequencies.
   */

  // tuning the low E string
  if (stringSelected == 7 ) {       
    if (analogRead(0) < 805) {      // if the sound is too quiet (intensity too low), don't activate the servo (just background noise, not a string bring played)
      servo.write(90);              // 90 makes the servo remain at rest
    }
    else {
      tune(peak, 250, 255);         // check if the pitch is between 250 and 255 Hz: if not, activate the servo to tune the string. if so, turn on the green DONE LED
    } 
  }

  // tuning the A string
  else if (stringSelected == 8) {   
    if (analogRead(0) < 805) {      // if the sound is too quiet (intensity too low), don't activate the servo (just background noise, not a string bring played)
      servo.write(90);              // 90 makes the servo remain at rest
    }
    else {
      tune(peak, 112, 115);         // check if the pitch is between 250 and 255 Hz: if not, activate the servo to tune the string. if so, turn on the green DONE LED
    }
  } 

  // tuning the D string
  else if (stringSelected == 9) {  
    if (analogRead(0) < 805) {      // if the sound is too quiet (intensity too low), don't activate the servo (just background noise, not a string bring played)
      servo.write(90);              // 90 makes the servo remain at rest
    }
    else {
      tune(peak, 297, 302);         // check if the pitch is between 297 and 302 Hz: if not, activate the servo to tune the string. if so, turn on the green DONE LED
    }
  } 

  // tuning the G string
  else if (stringSelected == 10) {  
    if (analogRead(0) < 805) {      // if the sound is too quiet (intensity too low), don't activate the servo (just background noise, not a string bring played)
      servo.write(90);              // 90 makes the servo remain at rest
    }
    else {
      tune(peak, 194, 198);         // check if the pitch is between 194 and 198 Hz: if not, activate the servo to tune the string. if so, turn on the green DONE LED
    }
  }   

  // tuning the B string
  else if (stringSelected == 11) {  
    if (analogRead(0) < 805) {      // if the sound is too quiet (intensity too low), don't activate the servo (just background noise, not a string bring played)
      servo.write(90);              // 90 makes the servo remain at rest
    }
    else {
      tune(peak, 250, 255);         // check if the pitch is between 250 and 255 Hz: if not, activate the servo to tune the string. if so, turn on the green DONE LED
    }
  } 

  // tuning the high E string
  else if (stringSelected == 12) {  
    if (analogRead(0) < 805) {      // if the sound is too quiet (intensity too low), don't activate the servo (just background noise, not a string bring played)
      servo.write(90);              // 90 makes the servo remain at rest
    }
    else {
      tune(peak, 333, 336);         // check if the pitch is between 333 and 336 Hz: if not, activate the servo to tune the string. if so, turn on the green DONE LED
    }   
  }
}


void tune(float peak, int minFreq, int maxFreq) {
  if (peak <= minFreq) {      // if the pitch of the string is too low:
    servo.write(60);          // make the servo rotate CCW for 700ms
    delay(1000);
    servo.write(90);          // stop servo
  }
  else if (peak >= maxFreq) { // if the pitch of the string is too high:
    servo.write(120);         // make the servo rotate CW for 700ms
    delay(1000);
    servo.write(90);          // stop servo
  }
  else {
    servo.write(90);          // if the pitch is not too high and not too low (in tune):
    digitalWrite(13, HIGH);   // turn on the green DONE LED
    while (digitalRead(6) == LOW) {
      // do nothing (keep servo stationary, don't calculate FFT), wait until select button is pressed to rotate to the next string to tune
    }
  }
}
