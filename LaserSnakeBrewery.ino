#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>

#define ONE_WIRE_BUS 13
#define DEBUG true

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

double setTemp = 20;
const int increaseButtonPin = 8;
const int decreaseButtonPin = 7;

void setup() {
  #if (DEBUG)
    Serial.begin(9600);
  #endif

  pinMode(increaseButtonPin, INPUT);
  pinMode(decreaseButtonPin, INPUT);

}

void loop() {
  sensors.requestTemperatures();
  Serial.print("Set Temp: ");
  Serial.print(setTemp);
  Serial.print("  ");
  Serial.print("Actual Temp: ");
  Serial.println(sensors.getTempCByIndex(0));
  if (digitalRead(increaseButtonPin) == HIGH) {
    setTemp += 0.5;
  } else if (digitalRead(decreaseButtonPin) == HIGH) {
    setTemp = setTemp - 0.5;
  }

}
