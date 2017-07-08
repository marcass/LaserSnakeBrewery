#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>

#define ONE_WIRE_BUS 13
#define DEBUG true

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


/*Might want to get address of 1-wire sensors so you can assign a sensible name if
* you have more than one (eg, vat, air, whatever else)
* check http://henrysbench.capnfatz.com/henrys-bench/arduino-temperature-measurements/ds18b20-arduino-user-manual-introduction-and-contents/ds18b20-user-manual-part-2-getting-the-device-address/
* for info on how to print id to serial console for variable assignment.
* Alternativley, if only one sensor then don't need to!
*/

const float THRESH = 0.5; //not sure what you want as imposed hysteresis compensation value

//short cycle timer
unsigned long RUN_THRESH = 12000; //2min in milliseconds

//bool for short cycle
bool can_turn_off = true;

double setTemp = 20;
const int increaseButtonPin = 8;
const int decreaseButtonPin = 7;
//variable for holding vat temp so we can do things with it
float vat_temp; //float equiv to double(i think), but makes more sense with syntax of float vs int

//states to pass to action funtion declared here. State machines great for managing stateful outputs
const int STATE_IDLE = 0;
const int STATE_COOL = 1;
const int STATE_HEAT = 2;
int state = STATE_IDLE; //initialise by in idle mode

//debounce stuff
unsigned long debounce_start = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  #if (DEBUG)
    Serial.begin(9600);
  #endif

  pinMode(increaseButtonPin, INPUT);
  //don't forget to set pins so they aren't floating, eg
  //digitalWrite(increaseButtonPin, HIGH); //and same for other button
  pinMode(decreaseButtonPin, INPUT);

}

//void checkButtons(){
//  if(digitalRead(increaseButtonPin) == LOW){
//    if (debounce_start == 0) {
//      debounce_start = millis();
//    }
//    if ( (millis() - debounce_start) > debounceDelay) {
//      setTemp += 0.5;
//      debounce_start = 0; //reset debounce timer
//    }
//  }
//  if(digitalRead(decreaseButtonPin) == LOW){
//    if (debounce_start == 0) {
//      debounce_start = millis();
//    }
//    if ( (millis() - debounce_start) > debounceDelay) {
//      setTemp -= 0.5;
//      debounce_start = 0; //reset debounce timer
//    }
//  }
//}
    
void doShit(int action) { //passes required control to fucntion so you don't need to write timers twice
  //start timer so don't short cycle
  start_time = millis();
  //set bool flag to false to stop short cycling
  can_turn_off = false;
  if(action == cool) {
    //write fridge relay pin high here (if active high)
  }
  if(action == heat) {
    //write fridge relay pin high here (if active high)
  }
}

void proc_idle() {
  if (vat_temp > (setTemp + THRESH) {
    doShit(cool);
  }
  if (vat_temp < (setTemp - THRESH) {
    doShit(heat);
  }
  //write digital pins low here for relays
}

void proc_heat() {
  if (millis() - start_time > RUN_THRESH) {
    can_turn_off = true;
    start_time = 0;
  }
  if (vat_temp > setTemp) {
    //check against timer so not short cycling
    if (can_turn_off) {
      state = STATE_IDLE;
    }else{
      //twiddle thumbs and keep doing shit
    }
  }
}

void proc_cool() {
  if (millis() - start_time > RUN_THRESH) {
    can_turn_off = true;
    start_time = 0;
  }
  if (vat_temp < setTemp) {
   if (can_turn_off) {
      state = STATE_IDLE;
    }else{
      //twiddle thumbs and keep doing shit
    }
  }
}

void loop() {
  //good idea to do call a safety function in loop, eg
  // 5 deg away from set point - kill everything
  // no temp reading for x time - kill everything
  sensors.requestTemperatures();
  Serial.print("Set Temp: ");
  Serial.print(setTemp);
  Serial.print("  ");
  Serial.print("Actual Temp: ");
  vat_temp = sensors.getTempCByIndex(0); //assuming only 1 probe
  Serial.println(vat_temp);
  //Serial.println(sensors.getTempCByIndex(0));
  /*you'll need to debounce the button otherwise crazy shit will happen when you press it
  * check out https://www.arduino.cc/en/Tutorial/Debounce
  * can software debounce (using mills() as a timer, or hardware debounce with a cap
  * also if you initialise the buttons with "pinMode(<button>, INPUT_PULLUP); in setup
  * it pulls the button high without need for external pullup (or pulldown) resistors and you poll for a 
  * (digitalRead(<button>) == LOW) in loop to do something, eg
  *checkButtons(); //see above function
  */
  if (digitalRead(increaseButtonPin) == HIGH) {
    setTemp += 0.5;
  } else if (digitalRead(decreaseButtonPin) == HIGH) {
    setTemp = setTemp - 0.5;
  }

  //manage states - can't cool when heating or vice versa
  switch (state) {
    case STATE_IDLE:
      proc_idle();
      break;
    case STATE_HEAT:
      proc_heat();
      break;
    case STATE_COOL:
      proc_cool();
      break;
  }
}
