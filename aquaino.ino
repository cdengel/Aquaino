/* Aquaino
 - WHAT IT DOES
 - SEE the comments after "//" on each line below
 - CONNECTIONS:
   -
   -
 - V1.00 05/29/2020
   Questions: cdengel@gmail.com 
   
  The LCD circuit:
  LCD VSS pin to ground
  LCD VCC pin to 5V
  10K pPOT: ends to +5V and ground; wiper to LCD VO pin 
  LCD RS pin to digital pin 2
  LCD R/W pin to ground
  LCD Enable pin to digital pin 3
  LCD D4 pin to digital pin 4
  LCD D5 pin to digital pin 5
  LCD D6 pin to digital pin 6
  LCD D7 pin to digital pin 7
  LCD A pin to 5V
  LCD K pin to ground

  Canopy Temp
  DHT 11
  Signal S (leftpin) Analog 0
  Middle pin to 5v
  Right to ground

  Float Switch
  lead to digital pin 8
  load to ground

  stepper Wiring:
  Pin 22 to IN1 on the ULN2003 driver
  Pin 24 to IN2 on the ULN2003 driver
  Pin 26 to IN3 on the ULN2003 driver
  Pin 28 to IN4 on the ULN2003 driver
  power?
  
 */

/*-----( Import needed libraries )-----*/
#include <LiquidCrystal.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Stepper.h>

/*-----( Declare Constants and Pin Numbers )-----*/
#define DHTPIN A0       // Set DHT pin:
#define DHTTYPE DHT11     // DHT 11

const String PROG   = "Aquaino";
const String VER    = "1.0";
const int ON        = 0;
const int OFF       = 1;
const bool VERBOSE  = true;
 
/*-- (display) --*/
const int LCDROWS       = 2; //LCD Parameters
const int LCDCOLS       = 16;
const int rs = 2, en = 3, d4 = 4, d5 = 5, d6 = 6, d7 = 7; //Pins for LCD
const int CUSTOMCHARS   = 8;
const int SCROLLSPEED   = 8;

/*-- (feeder Stepper) --*/
const int STEPSPEREVOLUTION = 2048; // Define number of steps per rotation:
const int STEPRPM = 10; // Define rpm:


/*-- (sensors) --*/
// Water sensors
const int FLOAT_SENSOR  = 8; //Float Switch Pin
const int WATER_PROBE   = 9; // Water Probe - Arduino pin that is connected to DS18B20 sensor's DQ pin

/*-- (relays) --*/
const int RELAYSIZE   = 8;
const int FILTER      = 0;  //Filter is 1st position in electrical relay
const int LIGHT       = 1;  //Light is 2nd position in electrical relay
const int HEATER      = 2;  //Heater is 3rd position in electrical relay
const int POWERHEAD1  = 3;  //Power head #1 is 4th position in electrical relay
const int POWERHEAD2  = 4;  //Power head #2 is 5th position in electrical relay
const int AIR1        = 5;  //Air Pump #1 is 6th position in electrical relay
const int TOPOFF      = 6;  //Air Pump #2 is 7th position in electrical relay 
const int AUX         = 7;  //Auxiliary power is 8th position in electrical relay
const int RELAY1  = 30;  //Defined Pin 30 as the Variable for filter
const int RELAY2  = 32;  //Defined Pin 32 as the Variable for Light
const int RELAY3  = 34;  //Defined Pin 34 as the Variable for Heater
const int RELAY4  = 36;  //Defined Pin 36 as the Variable for Powerhead 1
const int RELAY5  = 38;  //Defined Pin 38 as the Variable for Powerhead 2
const int RELAY6  = 40;  //Defined Pin 40 as the Variable for Air Pump
const int RELAY7  = 42;  //Defined Pin 42 as the Variable for top off pump
const int RELAY8  = 44;  //Defined Pin 44 as the Variable as auxiliary power
const int STEPIN1 = 22; //Pin 22 to Stepper LN1    
const int STEPIN2 = 24; //Pin 24 to Stepper LN2 
const int STEPIN3 = 26; //Pin 26 to Stepper LN3 
const int STEPIN4 = 28; //Pin 28 to Stepper LN4 

/*-----( Declare objects & Classes)-----*/
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);  // Creates an LCD object
DHT dht = DHT(DHTPIN, DHTTYPE);       // Initialize DHT sensor for normal 16mhz Arduino:
OneWire oneWire(WATER_PROBE);             // setup a oneWire instance
DallasTemperature sensors(&oneWire);    // pass oneWire to DallasTemperature library
Stepper feedStepper = Stepper(STEPSPEREVOLUTION, STEPIN1, STEPIN3, STEPIN2, STEPIN4); // Create stepper object called 'feedStepper', note the pin order 1,3,2,4


/*-----( Declare Global Variables )-----*/
String printString;
String line1;
String line2;
boolean flip=false;
int timeDelay     = 1000;
int relayDelay    = 500;
int WaterLevel    = 0;      // Water Level
int steps = 0;
int stepsPerSecond = 0;

//feeding parameters
int revOfFeed = 2;
int feedPauseMS = 5000;

float WaterTempC    = 0.0;    // temperature in Celsius
float WaterTempF    = 0.0;    // temperature in Fahrenheit
float HoodHumidity  = 0.0;
float HoodTempC     = 0.0;
float HoodTempF     = 0.0;
float h = 0.0;

int myRelays[RELAYSIZE] = {RELAY1, RELAY2, RELAY3, RELAY4, RELAY5, RELAY6, RELAY7, RELAY8};

char equip1[] = "Filter";
char equip2[] = "Light";
char equip3[] = "Heater";
char equip4[] = "Powerhead1";
char equip5[] = "Powerhead2";
char equip6[] = "Air1";
char equip7[] = "Topoff";
char equip8[] = "Aux";
char * equip[RELAYSIZE] = { equip1,equip2,equip3,equip4,equip5,equip6,equip7,equip8 };

/****** SETUP: RUNS ONCE ******/
void setup() { 
  Serial.begin(9600);                 // Begin serial communication at a baud rate of 9600:
  pinMode(FLOAT_SENSOR, INPUT_PULLUP);//Arduino Internal Resistor 10
  lcd.begin(LCDROWS, LCDCOLS);        // initialize & specify the LCD's number of columns and rows:
  dht.begin();                        // initialize hood sensor
  sensors.begin();                    // initialize the water sensor
  delay(1000);
  
  //establish relays pins and turn off 
  for(int x=0;x<RELAYSIZE;x++){
    pinMode(myRelays[x], OUTPUT);   //Set Relay Channel Pins as OUTPUT
    digitalWrite(myRelays[x], OFF); //Set Relays off to begin
  }
  
  feedStepper.setSpeed(STEPRPM); // Set the speed to STEPRPM rpm:
  
  HoodHumidity = dht.readHumidity();// Read the humidity in %:
  HoodTempC = dht.readTemperature();// Read the temperature as Celsius:
  HoodTempF = dht.readTemperature(true);// Read the temperature as Fahrenheit:

  sensors.requestTemperatures();             // send the command to get temperatures
  WaterTempC = sensors.getTempCByIndex(0);  // read temperature in Celsius
  WaterTempF = WaterTempC * 9 / 5 + 32; // convert Celsius to Fahrenheit

  // Check if any reads failed and exit early (to try again):
  if (isnan(HoodHumidity) || isnan(HoodTempC) || isnan(HoodTempF)) {
    Serial.println(" Setup Failed to read from DHT sensor!");
    return;
  }
  
  printToLCD2(PROG, ("Version: "+VER) );
  delay(1000);
  
} //--(end setup )---

/****** LOOP: RUNS CONSTANTLY ******/
void loop() {
  //get readings from all sensors
 
  getAirTemp();
  line1 = "Hood Humid:";
  line2 = HoodHumidity;
  line2 += "%";
  printToLCD2(line1,line2);
  delay(timeDelay);
  
  line1 = "Hood Temp:";
  line2 = HoodTempF;
  line2 += "F";
  printToLCD2(line1,line2);
  delay(timeDelay);

  getWaterTemp();
  line1 = "Water Temp:";
  line2 = WaterTempF;
  line2 += "F";
  printToLCD2(line1,line2);
  delay(timeDelay);


  getWaterLevel();
  line1 = "Water Level: ";
  if (WaterLevel == HIGH) { 
    line2 = "High";
  } 
  else { 
    line2 = "Low";
  } 
  printToLCD2(line1,line2);
/*
  //cycle all relays on and off
  for(int x=0;x<RELAYSIZE;x++){
    onOrOff(myRelays[x], ON);
    delay(relayDelay);
    onOrOff(myRelays[x], OFF);
    delay(relayDelay);
  }
 */
 
  delay(timeDelay);
  performFeeding();
 
}//--(end main loop )---

void performFeeding(){
  line1 = "Feeding";
  line2 = "";
  printToLCD2(line1,line2);
  //turn off current
  //run feeder
  activateFeeder();
  //pause
  line1 = "Pausing after feeding";
  line2 = "";
  printToLCD2(line1,line2);
  delay(feedPauseMS);
  //turn on all
}

void activateFeeder(){
  int steps = STEPSPEREVOLUTION * revOfFeed;
  feedStepper.step(steps);
}

void printToLCD2(String l1, String l2){
  if(VERBOSE) Serial.println("in printToLCD2()" + l1 +" " + l2);
  lcd.clear();
  lcd.setCursor(0, 0); 
  lcd.print(l1);
  lcd.setCursor(0, 1);  
  lcd.print(l2);
}

void getAirTemp(){
  if(VERBOSE) Serial.println("in getAirTemp()");
  HoodHumidity = dht.readHumidity();
  HoodTempC = dht.readTemperature();    // Read the temperature as Celsius:
  HoodTempF = dht.readTemperature(true);  // Read the temperature as Fahrenheit:
 //  if(VERBOSE){ printString = "HoodHumidity=" + HoodHumidity + "% HoodTempC="+HoodTempC + " HoodTempF="+HoodTempF; Serial.println(printString);}
}

void getWaterTemp(){
  if(VERBOSE) Serial.println("in getWaterTemp()");
  sensors.requestTemperatures();              // send the command to get temperatures
  WaterTempC = sensors.getTempCByIndex(0);  // read temperature in Celsius and set global WaterTempC
  WaterTempF = WaterTempC * 9 / 5 + 32;     // convert Celsius to Fahrenheit and set global WaterTempF
  //if(VERBOSE){ printString = "WaterTempC=" + WaterTempC + " WaterTempF="+WaterTempF; Serial.println(printString);}
}

void getWaterLevel(){
  if(VERBOSE) Serial.println("in getWaterLevel()");
  WaterLevel = digitalRead(FLOAT_SENSOR);
  if(VERBOSE){ Serial.println("in getWaterLevel() WaterLevel=" + WaterLevel);}
}

void onOrOff(int r, int on){
  char str[80];
  
  //if(VERBOSE){ Serial.print("in onOrOff() r:");Serial.println(r);}
  if(on){
    sprintf(str, "Turning OFF relay=%d", r);
    digitalWrite(r, HIGH);   //Turn on relay
  } else {
    sprintf(str, "Turning ON relay=%d", r);
    digitalWrite(r, LOW);  //Turn OFF relay
  }
  Serial.println(str);
  delay(4000);
}

char * pinToEquip(int pin){
  char equipname[] ="UNKNOWN";
//bool found = false;
/*  for(int e=0;e<RELAYSIZE;e++){
      if ( myRelays[e] == pin){
        sprintf(equipname, "%d", equip[e]);
        //equipname = equip[e];
        //found = true;
        break;
      }
  }
*/
  return equipname;
}
