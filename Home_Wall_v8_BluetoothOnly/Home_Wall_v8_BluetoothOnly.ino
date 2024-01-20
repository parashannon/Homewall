
/* Features
  Set climb to a certain number (brightness)
  Flip climb on board (red->blue)
  Toggle a single light
  Set all lights to off turn off problem
  Save a climb to memory

*/
#include <ArduinoBLE.h>

char command_in[256];
int max_packet = 256;
bool new_line;
int line_to_update;
int command_from = 0;
const int ledPin = LED_BUILTIN;  // set ledPin to on-board LED
const int buttonPin = 4;         // set buttonPin to digital pin 4
unsigned long loop_count = 0;
#define DATA_PIN 3

BLEService WallServe("112");  // create service

// create switch characteristic and allow remote device to read and write

BLEIntCharacteristic Problem_Number("001", BLERead | BLEWrite);  //840b
BLELongCharacteristic ToggleLED("002'", BLERead | BLEWrite);     // 13D
BLELongCharacteristic FlipProblem("003", BLERead | BLEWrite);    //fb
BLEIntCharacteristic SaveProblem("004", BLERead | BLEWrite);     //fb
BLEIntCharacteristic RandomProblem("005", BLERead | BLEWrite);   //fb

bool flip_problem = false;
int ProblemNumber;

unsigned long t_last_LCD = 0;

String command;
String TempString;
int LED_row_column_i;
bool command_received = false;

unsigned long t_update;
unsigned long t_current;
bool onboard_light = false;

void setup() {
  delay(2000);
  t_update = millis() + 1000;
  pinMode(7, OUTPUT);     // sets the digital pin 13 as output
  digitalWrite(7, HIGH);  // sets the digital pin 13 on
  pinMode(LED_BUILTIN, OUTPUT);


  Serial.begin(115200);
  delay(100);
  Serial1.begin(115200);
  delay(100);

 

  //pinMode(ledPin, OUTPUT);  // use the LED as an output

  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1)
      ;
  }

  // set the local name peripheral advertises

  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(WallServe);

  // add the characteristics to the service
  WallServe.addCharacteristic(ToggleLED);
  WallServe.addCharacteristic(Problem_Number);
  WallServe.addCharacteristic(FlipProblem);
  WallServe.addCharacteristic(SaveProblem);
  WallServe.addCharacteristic(RandomProblem);
  // add the service
  BLE.addService(WallServe);

  FlipProblem.writeValue(0);
  BLE.setLocalName("HomeWall2");
  BLE.setDeviceName("HomeWall2");

  BLE.advertise();

  Serial.println("Bluetooth® device active, waiting for connections...");
  Serial.println("Awaiting your command");
  delay(100);

  Serial.println("Setup Complete");
  Serial.flush();
}

void loop() {



  if (ToggleLED.written()) {
    Serial.println("Bluetooth Command T");
    Serial.println(ToggleLED.value());
    command_received = true;
    command = "T" + String(ToggleLED.value());
  }
  if (FlipProblem.written()) {
    Serial.println("Bluetooth Command F");
    Serial.println(FlipProblem.value());
    command_received = true;
    command = "F";
  }

  if (Problem_Number.written()) {
    Serial.println("Bluetooth Command P");
    Serial.println(Problem_Number.value());
    command_received = true;
    command = "P" + String(Problem_Number.value());
  }

  if (SaveProblem.written()) {
    Serial.println("Bluetooth Command S");
    Serial.println(SaveProblem.value());
    command_received = true;
    command = "S" + String(SaveProblem.value());
  }

  if (RandomProblem.written()) {
    Serial.println("Bluetooth Command R");
    Serial.println(RandomProblem.value());
    command_received = true;
    command = "R" + String(RandomProblem.value());
  }

  GetCommandSerial();


  // handle commands from the other arduino and pass these to the LCD display

  if (command_received) {

    Serial1.println(command); // send command to LED arduino
    Serial1.flush();

    command_received = false;
  }
  // **************************************
  // check Bluetooth
  //*************************************
  BLE.poll();

  blink_light();
}


void blink_light() {
  t_current = millis();
  if (t_current > t_update) {
    if (onboard_light) {
      digitalWrite(LED_BUILTIN, HIGH);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
    }
    t_update = t_current + 1000;
    onboard_light = !onboard_light;
  }
}

void GetCommandSerial() {
  int i_buff = 0;
  i_buff = 0;
  for (int i = 0; i < max_packet; i = i + 1) {
    command_in[i] = 0;
  }

  i_buff = 0;
  while (Serial.available() > 0) {
    // read the incoming byte:
    command_in[i_buff] = Serial.read();
    delay(1);
    command_received = true;
    command_from = 0;
    if (command_in[i_buff] == 10) {
      break;  // if that is an end of line character, that's it for the command (for now)
    }
    i_buff++;

    if (i_buff >= max_packet) {
      break;
    }
  }
}