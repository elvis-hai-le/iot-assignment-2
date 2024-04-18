#include <Arduino.h>
#include <math.h> // used to calculate temperature

#include "passive_timer.h" //used to make passive timers
static PassiveTimer clock;
const unsigned long clockReset = 5000;

//initialisation for IR code
#include <DYIRDaikin.h>
DYIRDaikin irLED; // initialise ir aircon object

const int BUTTON_PIN = 2; // Pin used to detect button press and run interrupt
bool ecoMode = false;
bool lastEcoMode = false; //used to check last state
int buttonState;
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers


const int TEMP_PIN = A0; // Analog pin used for measuring temperature
bool acOn = false;
bool acCool = true;
int threshHot = 35; //How hot before cool ac is on
int threshCold = 23; // How cold before warm ac is on
int ecoThreshHot = 33; // Has to be hotter before ac is on
int ecoThreshCold = 18; // Has to be colder before ac is on
int threshOff = 25; //Temp which AC will turn off

const int PIN_RED   = 5;
const int PIN_GREEN = 9;
const int PIN_BLUE  = 6;
long clockLED = 0;

//colours for LED
int offLED[3] = {0,0,0};
int redLED[3] = {160,0,0};
int greenLED[3] = {0,50,20};
int blueLED[3] = {0,0,200};
int purpleLED[3] = {120,0,255};
int yellowLED[3] = {200,50,0};

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(PIN_RED,  OUTPUT);
  pinMode(PIN_GREEN,  OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  irLED.begin();
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if(reading == HIGH) {
        if (ecoMode) {
          blinkLED(purpleLED, 2, 200, 100);
        } else {
          blinkLED(greenLED, 2, 200, 100);
        }
        ecoMode = !ecoMode;
      }
    }
  }
  lastButtonState = reading;

  checkAC();
  checkCommands();
  printTemp();
}

void turnOnCold() { //sends IR signal to turn on AC cool setting
  //Serial.println("Turning on AC cooling");
  setColour(blueLED);
  irLED.on();
	irLED.setSwing_off();
	irLED.setMode(1);
	irLED.setFan(1);//FAN speed to low, this aircon has a strong fan
	irLED.setTemp(18);
	irLED.sendCommand();
  blinkLED(blueLED, 3, 50, 80);
  acOn = true;
  acCool = true; 
  Serial.flush();
} 

void turnOnHeat() { //sends IR signal to turn on AC warm setting
  //Serial.println("Turning on AC heating");
  setColour(redLED);
  irLED.on();
	irLED.setSwing_off();
	irLED.setMode(3);
	irLED.setFan(1);//FAN speed to low 
	irLED.setTemp(30);
	irLED.sendCommand();
  blinkLED(redLED, 3, 50, 80);
  acOn = true;
  acCool = false; 
  Serial.flush();
} 

void turnOff() { //sends IR signal to power off AC
  //Serial.println("Turning off AC");
  irLED.off();
  irLED.sendCommand();
  blinkLED(yellowLED, 3, 500, 300);
  acOn = false;
  Serial.flush();
} 

double currentTemp() { //calculates current temperature for temp sensor module
  double val=analogRead(TEMP_PIN);
  double fenya=(val/1023)*5;
  double r=(5-fenya)/fenya*4700;
  double currentTemp = ( 1/(  log(r/10000) /3950 + 1/(25+273.15))-273.15);
  return currentTemp;
}

void setColour(int rgb[3]) {
  analogWrite(PIN_RED,  rgb[0]);
  analogWrite(PIN_GREEN,  rgb[1]);
  analogWrite(PIN_BLUE, rgb[2]);
}

void blinkLED(int rgb[3], int count, int offTime, int onTime) {
  int stateLED = true;
  for (int i = 0; i < count; ) {
    if (stateLED) {
      if( (millis()- clockLED) >= onTime) {
        setColour(rgb);
        stateLED = !stateLED;// change the state of LED
        clockLED=millis();// remember Current millis() time
        }
    } else {   
      if( (millis()- clockLED) >= offTime) {
        setColour(offLED);     
        stateLED = !stateLED;// change the state of LED
        clockLED=millis();// remember Current millis() time
        i++;
      }
    }
  }
  setColour(offLED);
}

void checkAC() {
  if (!acOn) {
    if (!ecoMode) {
      if (currentTemp() >= threshHot) { turnOnCold(); }
      else if (currentTemp() <= threshCold) { turnOnHeat(); }
    } else if (ecoMode) {
      if (currentTemp() >= ecoThreshHot) { turnOnCold(); }
      else if (currentTemp() <= ecoThreshCold) { turnOnHeat(); }
    }
  } else if (acOn) {
    if (acCool) {
      if (currentTemp() <= threshOff) { turnOff(); }
    } else {
      if (currentTemp() >= threshOff) { turnOff(); }
    }
  }
}

void checkCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    if(command == "AC COOLING" && !acOn) {
      turnOnCold();
    } else if (command == "AC HEATING" && !acOn) {
      turnOnHeat();
    } else if (command == "ECO") {
      ecoMode = !ecoMode;
      if (ecoMode) {
        //Serial.println("Eco mode active");
      } else {
        //Serial.println("Eco mode deactivated");
      }
    } else if (command == "AC OFF" && acOn) {
      turnOff();
    }
  }
}

void printTemp() {
  if (clock.time_millis() >= clockReset) {
    clock.restart();
    Serial.println(currentTemp());
  }
}



