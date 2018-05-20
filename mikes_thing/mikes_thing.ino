#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// either debug OR production mode, as both use the serial 
#define DEBUG true
#define PRODUCTION false
#define ONE_WIRE_BUS A2
#define HEATER_RELAY A1

/*  tuning variables- tweak for more precise control
 *  Note: negative values mean below the set temperature.
 *  Be careful to ensure that variables do not overlap, to prevent
 *  competitve cycling
 */
float set_temp = 25; 
float heater_on_thresh = -1.0;
float heater_off_thresh = 0;

//set up non-tunable control variables
unsigned long RUN_THRESH = 60000; //1min in milliseconds, minimum time for heating/cooling elements to run
bool can_turn_off = true; //bool for short cycle timer
unsigned long start_time = 0; //variable for timing heating/cooling duration
unsigned long MAX_RUN_TIME = 1200000; //20 min - maximum time for heating element to be on
unsigned long RELAX_TIME = 300000; //5min 

//system states
const int STATE_ERROR = -1;
const int STATE_RELAX = 0;
const int STATE_IDLE = 1;
//const int STATE_COOL = 2;
const int STATE_HEAT = 3;
int state = STATE_IDLE; //initialise in idle

// set up 1-wire probes
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
/* Address of 1-wire sensors can be found by following the tutorial at 
 * http://henrysch.capnfatz.com/henrys-bench/arduino-tempera#define FRIDGE_RELAY 10
ture-measurements/ds18b20-arduino-user-manual-introduction-and-contents/ds18b20-user-manual-part-2-getting-the-device-address/
 */
//DeviceAddress AIR_TEMP_SENSOR = {0x28, 0xCC, 0x02, 0xAF, 0x03, 0x00, 0x00, 0xA8};
DeviceAddress VAT_TEMP_SENSOR = {0x28, 0xCC, 0x02, 0xAF, 0x03, 0x00, 0x00, 0xA8};

//set up temp measurement variables
//bool air_probe_connected;
bool vat_probe_connected;
//float air_temp;
float vat_temp;
unsigned long last_temp_request = 0;
bool waiting_for_conversion = false;
unsigned long CONVERSION_DELAY = 1000; //time allocated for temperature conversion
unsigned long MEAS_INTERVAL = 1000; //take temperature measurement every 1s

// Initialisations for myController. 
//const int VAT_ID = 1;
//const int AIR_ID = 2;
String vat_payload;
//String air_payload;


void setup() {
  Serial.begin(9600);
  
  #if (PRODUCTION)
    Serial.println("1;1;0;0;6");
    Serial.println("1;2;0;0;6");
  #endif
  
  // Set up temperature probes
//  sensors.setResolution(AIR_TEMP_SENSOR, 11); //resolution of 0.125deg cels, 
  sensors.setResolution(VAT_TEMP_SENSOR, 11); //takes approx 375ms
  if (sensors.getResolution(VAT_TEMP_SENSOR) == 0) {
    vat_probe_connected = false;
    state = STATE_ERROR;
  } else {
    vat_probe_connected = true;
  }
//  if (sensors.getResolution(AIR_TEMP_SENSOR) == 0) {
//    air_probe_connected = false;
//    state = STATE_ERROR;
//  } else {
//    air_probe_connected = true;
//  }
  /* Setting the waitForConversion flag to false ensures that a tempurate request returns immediately
   *  without waiting for a temperature conversion. If setting the flag to false, be sure to wait 
   *  the appropriate amount of time before retrieving the measurement, to allow time for the conversion
   *  to take place.
   */
  sensors.setWaitForConversion(false);
  #if (DEBUG) 
    Serial.print("Vat sensor resolution: ");
    Serial.println(sensors.getResolution(VAT_TEMP_SENSOR), DEC);
//    Serial.print("Air sensor resolution: ");
//    Serial.println(sensors.getResolution(AIR_TEMP_SENSOR), DEC);
  #endif

  //initialise relays
  pinMode(HEATER_RELAY, OUTPUT);
  digitalWrite(HEATER_RELAY, HIGH);
}

void perform_action(String action) {
  //start timer so don't short cycle
  start_time = millis();
//  if(action == "cool") {
//    can_turn_off = false;
//    #if (DEBUG)
//      Serial.println("...Turning fridge on");
//    #endif
//    digitalWrite(FRIDGE_RELAY, LOW);
//  } else 
  if(action == "heat") {
    can_turn_off = false;
    #if (DEBUG)
      Serial.println("...Turning heating on");
    #endif
    digitalWrite(HEATER_RELAY, LOW);
  } else if (action == "disable") {
    #if (DEBUG) 
      Serial.println("...Switching elements off");
    #endif
    digitalWrite(HEATER_RELAY, HIGH);
//    digitalWrite(FRIDGE_RELAY, HIGH);
  }
}

void proc_idle() {
  if (!vat_probe_connected) {
    state = STATE_ERROR;
  }
  if (vat_temp - set_temp < heater_on_thresh) {
    state = STATE_HEAT;
    perform_action("heat");
  }
}

void proc_heat() {
  // check if probes are connected
  if (!vat_probe_connected) {
    state = STATE_ERROR;
  }
  // check if minimum runtime has elapsed
  if ((millis() - start_time) > RUN_THRESH) {
    can_turn_off = true;
  }
  // check if conditions are right to turn heater off 
  if ((vat_temp - set_temp > heater_off_thresh) && can_turn_off) {
    state = STATE_IDLE;
    perform_action("disable");
  }
  // check if heater has been on for too long
  if ((millis() - start_time) > MAX_RUN_TIME) {
    #if (DEBUG) 
      Serial.println("Maximum heat time exceeded, entering STATE_RELAX");
    #endif
    state = STATE_RELAX;
    perform_action("disable");
  }
}

//void proc_cool() {
//  if (!vat_probe_connected || !air_probe_connected) {
//    state = STATE_ERROR;
//  }
//  if ((millis() - start_time) > RUN_THRESH) {
//    can_turn_off = true;
//  } //fridge to turn off at set_temp + THRESH
//  if ((vat_temp - set_temp < fridge_off_thresh) && can_turn_off) {
//    state = STATE_IDLE;
//    perform_action("disable");
//  }
//}

void proc_relax() {
  if (millis() - start_time > RELAX_TIME) {
    #if (DEBUG) 
      Serial.println("Relax time exceeded, entering STATE_IDLE");
    #endif
    state = STATE_IDLE;
  }
}

void error_handler() {
  if (millis() - start_time > RUN_THRESH) {
    can_turn_off = true;
    perform_action("disable"); 
  }
  // if probe comes back 
  if (vat_probe_connected) {
    if (can_turn_off) {
    #if (DEBUG)
      Serial.println("Exiting STATE_ERROR");
    #endif
      state = STATE_IDLE;
      //perform_action("disable");      
    }
  }
}

/* print status to Serial monitor, activated in debug mode only */
void status_update() {
  Serial.print("Vat temp:");
  Serial.println(vat_temp);
    if (state == STATE_HEAT) {
      Serial.print("Heater has been on for ");
      Serial.print((millis()-start_time)/1000);
      Serial.println(" seconds");
      if (can_turn_off) {
        Serial.println("Minimum time elapsed, heating can be turned off");
      }
    }else if (state == STATE_IDLE) {
      Serial.println("System is idle");
    } else if (state == STATE_ERROR) {
      Serial.println("ERROR");
      if (!vat_probe_connected) {
        Serial.println("Vat probe not connected");
      }
    }
}

void loop() {
  if (!waiting_for_conversion && (millis() - last_temp_request) > MEAS_INTERVAL) {
    vat_probe_connected = sensors.requestTemperaturesByAddress(VAT_TEMP_SENSOR);
    last_temp_request = millis();
    waiting_for_conversion = true;
  }
  if (waiting_for_conversion && millis()-last_temp_request > CONVERSION_DELAY) {
    vat_temp = sensors.getTempC(VAT_TEMP_SENSOR);
    waiting_for_conversion = false;
    
   /* sent data to myController. Detailed instructions can be
   *  found here: https://www.mysensors.org/download/serial_api_20
   */
//    #if (PRODUCTION)
//      vat_payload = vat_temp;
//      air_payload = air_temp;
//      Serial.println("1;1;1;0;0;" + vat_payload);
//      Serial.println("1;2;1;0;0;" + air_payload);
//    #endif
    
    //Send an update to the Serial monitor if DEBUG mode is on
    #if (DEBUG) 
      status_update();
    #endif
    
    // Manage states- can't heat while cooling and vice versa
    switch (state) {
      case STATE_IDLE:
        proc_idle();
        break;
      case STATE_HEAT:
        proc_heat();
        break;
//      case STATE_COOL:
//        proc_cool();
//        break;
      case STATE_RELAX:
        proc_relax();
        break;
      case STATE_ERROR:
        error_handler();
        break;    
    }
  }
}
 
  

