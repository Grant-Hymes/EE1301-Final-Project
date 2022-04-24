#include <Wire.h>
#include "VL53L1X.h"
#include <neopixel.h>

VL53L1X sensor;
#define LCD_I2Cadr 0x3e
byte contrast = 25;


unsigned long int timePrint;

// sets pin for speaker
int speakerPin = A4;

//Frequency for each note necessary:
int C = 523;
int A = 440;
int E = 659;
int G = 784;
int quarterNote = 2666;
int eighthNote = 1333;
int lastNote = 6665;
int rest = 500; //There will be a 0.5 sec delay between notes
 
//Notes in the melody:
const int melody[10] = {C,A,C,E,C,E,G,G,G,G};
 
const int noteDurations[10] = {eighthNote, eighthNote, quarterNote,
                         eighthNote, eighthNote, quarterNote,
                         quarterNote, eighthNote, quarterNote, lastNote};




// sets up the various modes for the trap
enum trapmode {
    LOOKINGFORBEAM, // device is turned on and is search for a beam
    BEAMFOUND, // device has found a beam and is ready to be armed
    ARMED, // device is armed, if the beam changes distance from the current distance it is set off
    TRIPPED, // the trap has been tripped
};

// initial trapmode which is set to looking for beam
trapmode mode = LOOKINGFORBEAM;

// sets up the settings for the neopixel
int PIXEL_PIN = D4;
int PIXEL_COUNT = 1;
int PIXEL_TYPE = WS2811;

// initial settings for state machines
int testButtonPrevState = LOW; 
int modeButtonPrevState = LOW;

// assigns pins for the various buttons
int modeButton = D3;
int testButton = D2;

// initalizes the lightbulb
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);


int armedDistance;


void setup() {

  Serial.begin(9600); // starts serial output

  Wire.begin();
  Wire.setClock(400000); // use 400 kHz I2C

  sensor.setTimeout(500);
  if (!sensor.init()) {
      Serial.println("Failed to detect and initialize sensor!");
      while (1);
  }

  sensor.setDistanceMode(VL53L1X::Long);
  sensor.setMeasurementTimingBudget(50000);

  sensor.startContinuous(50);

 noTone(speakerPin); //Begin with speaker off

  strip.begin(); // starts the lightstrip
  pinMode(testButton, INPUT_PULLDOWN); // sets up the test button
  pinMode(modeButton, INPUT_PULLDOWN); // sets up the mode button
  Particle.function("DisarmRemotely", remoteDisarm); // creates a cloud function that allows for the trap to be disarmed remotely

    timePrint = millis() + 500;

}

void loop() {

    unsigned long int currentTime = millis();

int currentDistance = int(sensor.read());

      if(currentTime > timePrint){  
        Serial.print(currentDistance);
        if (sensor.timeoutOccurred()) {
          Serial.print(" TIMEOUT");
        }
      timePrint = timePrint + 500;
      Serial.println();
      }






// changes the button status to their current readings
int currentModeButton = digitalRead(modeButton);

// sets up the various colors the lightbulb can display
int PixelColorWhite = strip.Color(   255 , 255, 255);
int PixelColorGreen  = strip.Color(  0,   255,   0);
int PixelColorRed = strip.Color(  255,  0,   0);
int PixelColorBlue = strip.Color(  0,  0,   255);

// changes the color of the lightbulb based on the devices current mode
if (mode == LOOKINGFORBEAM) {strip.setPixelColor(0, PixelColorWhite);}
else if (mode == BEAMFOUND) {strip.setPixelColor(0, PixelColorGreen);}
else if (mode == ARMED) {strip.setPixelColor(0, PixelColorRed);}
else if (mode == TRIPPED) {strip.setPixelColor(0, PixelColorBlue);}
strip.show();

// if the device is looking for a beam and it is found it changes to beam found
if (mode == LOOKINGFORBEAM) {

  if (300 < currentDistance && currentDistance < 4000) {
    mode = BEAMFOUND;
  }

}

// if the device has found a beam and it is lost mode is switched to searching for beam
// if the device has found a beam and the mode button is pressed the device is armed
else if (mode == BEAMFOUND) {

  if (currentDistance < 300 || 4000 < currentDistance) {
    mode = LOOKINGFORBEAM;
  }
  else if (modeButtonPrevState == false && currentModeButton == true) {
    mode = ARMED;
    armedDistance = currentDistance;
    Serial.print("Arming Distance set to: ");
    Serial.print(armedDistance);
    Serial.println();
  }

} 

// if the device is armed and the beam is broken, the trap is sprung
// the cloud is also notified of this event
else if (mode == ARMED) {

  int distanceDifference = armedDistance - currentDistance;
  if (distanceDifference < 0) {distanceDifference = distanceDifference * (-1);} 

  if (100 < distanceDifference) {

    mode = TRIPPED;
    Particle.publish("INTRUDERALERT");

  // plays the song
  for(int thisNote = 0; thisNote < 10; thisNote++) {
     tone(speakerPin, melody[thisNote],noteDurations[thisNote]);
     delay(rest);
   }
  noTone(speakerPin);

  }

}

// changes the previous button states to the current ones
modeButtonPrevState = currentModeButton;

}

// allows for the device to be disarmed remotely
int remoteDisarm(String inputString) {

    // if the string entered is disarmed the device is switched to an unarmed state
    if (inputString == "Disarm" && mode == ARMED) {
        mode = BEAMFOUND;
        return 1;
    }

    // activates if user enters an invalid mode or attempts to disarmed the device when it isn't in an armed state
    else {
        Serial.print("Device not Armed or an Invalid Mode has been Entered: ");
        Serial.println(inputString);
        return -1;
    }

}