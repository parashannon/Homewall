
/* Features
  Set climb to a certain number (brightness)
  Flip climb on board (red->blue)
  Toggle a single light
  Set all lights to off turn off problem
  Save a climb to memory

*/
#include <ArduinoBLE.h>
int problem_array[20];
char command_in[256];
char serial_message[256];
int max_packet = 256;
bool new_line;
int line_to_update;
int command_from = 0;
const int ledPin = LED_BUILTIN;  // set ledPin to on-board LED
const int buttonPin = 4;         // set buttonPin to digital pin 4
unsigned long loop_count = 0;
unsigned long t_reset_BLE = 0;
int problem_index=0;;
bool message_rx = false;
bool array_update=false;


//void(* resetFunc) (void) = 0; // create a standard reset function


#define DATA_PIN 3

BLEService WallServe("112");  // create service

// create switch characteristic and allow remote device to read and write

BLEIntCharacteristic Problem_Number("001", BLERead | BLEWrite);  //840b
BLELongCharacteristic ToggleLED("002'", BLERead | BLEWrite);     // 13D
BLELongCharacteristic FlipProblem("003", BLERead | BLEWrite);    //fb
BLEIntCharacteristic SaveProblem("004", BLERead | BLEWrite);     //fb
BLEIntCharacteristic RandomProblem("005", BLERead | BLEWrite);   //fb
BLELongCharacteristic LightStatus("006'", BLERead | BLEWrite );   // 13D


// *****************************NEW**************************************
char ArrayInputValue[20]; // Array to store incoming data
BLECharacteristic CharInput("007", BLERead | BLEWrite, sizeof(ArrayInputValue)); // 20 characters maximum
// *****************************NEW**************************************


// 115200

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
  delay(1000);
  t_update = millis() + 1000;
  pinMode(7, OUTPUT);     // sets the digital pin 13 as output
  digitalWrite(7, HIGH);  // sets the digital pin 13 on
  pinMode(LED_BUILTIN, OUTPUT);


  Serial.begin(115200);
  delay(100);
  Serial1.begin(115200);
  delay(100);

  for (int i =1; i < 20; i++){
    ArrayInputValue[i]=' ';
  }



  //pinMode(ledPin, OUTPUT);  // use the LED as an output



  // set the local name peripheral advertises

  start_BLE();

  Serial.println("Bluetooth® device active, waiting for connections...");
  Serial.println("Awaiting your command");
  Serial.println("Setup Complete");
  Serial.flush();

  t_reset_BLE = millis();
}

void loop() {
  if ((millis() - t_reset_BLE) > 3600000) {
    t_reset_BLE = millis();
    //start_BLE();
    Serial.println("Rebooting");
    delay(1000);
    reboot_function();
  }


  if (ToggleLED.written()) {
    Serial.println("Bluetooth Command T");
    Serial.println(ToggleLED.value());
    command_received = true;
    command = ":T" + String(ToggleLED.value());
  }
  if (FlipProblem.written()) {
    Serial.println("Bluetooth Command F");
    Serial.println(FlipProblem.value());
    command_received = true;
    command = ":F";
  }

  if (Problem_Number.written()) {
    Serial.println("Bluetooth Command P");
    Serial.println(Problem_Number.value());
    command_received = true;
    command = ":P" + String(Problem_Number.value());
  }

  if (SaveProblem.written()) {
    Serial.println("Bluetooth Command S");
    Serial.println(SaveProblem.value());
    command_received = true;
    command = ":S" + String(SaveProblem.value());
  }

  if (RandomProblem.written()) {
    Serial.println("Bluetooth Command R");
    Serial.println(RandomProblem.value());
    command_received = true;
    command = ":R" + String(RandomProblem.value());
  }

  // *****************************NEW**************************************
  // Check if the new ArrayInput characteristic has been written to
  if (CharInput.written()) {
    for (int i =1; i < 20; i++){
      ArrayInputValue[i]=' ';
    }
    CharInput.readValue(ArrayInputValue, sizeof(ArrayInputValue));
    Serial.println("Bluetooth Command Array Input");
    // Assuming you want to print the received value
    // Note: Direct access to ArrayInputValue is possible since it's the storage for the characteristic
    Serial.write(ArrayInputValue, sizeof(ArrayInputValue)); // This prints the array as received
    Serial.println(); // New line for clarity
    
    Serial1.write(ArrayInputValue, sizeof(ArrayInputValue)); // This prints the array as received
    Serial1.println();
    command_received = false;  // Update this as needed based on how you handle the data
  }
  // *****************************NEW**************************************

    

  GetCommandSerial();
  //GetSerial1();

  // handle commands from the other arduino and pass these to the LCD display

  if (command_received) {

    if (command_in[0]=='r' && command_in[1]=='e' && command_in[2]=='b'){
      Serial.println("Rebooting");
      delay(100);
      reboot_function();
    }

    Serial1.println(command);  // send command to LED arduino
    Serial1.flush();

    command_received = false;

  
  }

  /*
  if(array_update){
    problem_index=0;
    LightStatus.writeValue(problem_array[problem_index]);
    problem_index++;
  }

  if (problem_index<20){
    if (LightStatus.read()){
      LightStatus.writeValue(problem_array[problem_index]);
      problem_index++;
    }
  }
  */
  // **************************************
  // check Bluetooth
  //*************************************
  BLE.poll();

  blink_light();
  message_rx = false;
  array_update=false;
}

void reboot_function(){
  NVIC_SystemReset(); 
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

void GetSerial1() {
  int i_buff = 0;
  i_buff = 0;

  for (int i = 0; i < max_packet; i = i + 1) {
    serial_message[i] = 0;
  }

  i_buff = 0;
  while (Serial1.available() > 0) {
    // read the incoming byte:
    message_rx = true;
    serial_message[i_buff] = Serial1.read();
    delay(1);

    if (serial_message[i_buff] == 10) {
      break;  // if that is an end of line character, that's it for the message (for now)
    }
    i_buff++;

    if (i_buff >= max_packet) {
      break;
    }
  }

  if (message_rx) {
    // Serial.println(serial_message);
    /*
    if (serial_message[0] == 124) {
      //Serial.println("This is a problem line");
      parse_problem() ;
      for (int i_indx = 0; i_indx < 20; i_indx++) {
        Serial.print(problem_array[i_indx]);
        Serial.print(" ");
      }

    }
    */
  }
}

void parse_problem() {
  int array_index;
  int ichar;
  int array_val;
  int intsign;
  for (int i_indx = 0; i_indx < 20; i_indx++) {
    problem_array[i_indx] = 0;

  }
  //Serial.println("Parse");
  
  ichar = 0;
  while (ichar < 256) {
  
    if (serial_message[ichar] == 10 || serial_message[ichar] == 0 || serial_message[ichar] == 13) {
      //Serial.println("EOL of some flavor detected");
      break;
    }
    if (serial_message[ichar] == 124) {  // | character
      ichar++;
    }
//Serial.println("Parsing Values");
    array_index = 0;
    while (serial_message[ichar] > 47 && serial_message[ichar] < 58) {
      array_index = array_index * 10 + serial_message[ichar] - 48;
      ichar++;
    }
    //Serial.print(array_index);
    //Serial.print(" ");
    // the last line of the above was a space
    ichar++;
    intsign = 1;
    array_val = 0;
    while (serial_message[ichar] > 47 && serial_message[ichar] < 58 || serial_message[ichar] == 45) {

      if (serial_message[ichar] == 45) {
        intsign = -1;

      } else {
        array_val = array_val * 10 + serial_message[ichar] - 48;
      }
      ichar++;
    }
    array_val = array_val * intsign;
    //ichar = ichar - 1;  // go back to the last thing
    //Serial.println(array_val);
    if (array_index >= 0 && array_index < 20) {
      problem_array[array_index] = array_val;
    }
    ichar++;
    array_update=true;
    //problem_array[];
  }

}

void start_BLE() {

  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1)
      ;
  }
  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(WallServe);

  // add the characteristics to the service
  WallServe.addCharacteristic(ToggleLED);
  WallServe.addCharacteristic(Problem_Number);
  WallServe.addCharacteristic(FlipProblem);
  WallServe.addCharacteristic(SaveProblem);
  WallServe.addCharacteristic(RandomProblem);
  WallServe.addCharacteristic(LightStatus);

  // *****************************NEW**************************************
  // Add the new characteristic to the service
  WallServe.addCharacteristic(CharInput);
  // *****************************NEW**************************************


  
  // add the service
  BLE.addService(WallServe);

  FlipProblem.writeValue(0);
  BLE.setLocalName("HomeWall2");
  BLE.setDeviceName("HomeWall2");

  BLE.advertise();
}
