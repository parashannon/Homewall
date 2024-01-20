
/* Features
  Set climb to a certain number (brightness)
  Flip climb on board (red->blue)
  Toggle a single light
  Set all lights to off turn off problem
  Save a climb to memory

*/
#include <ArduinoBLE.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the LCD address to 0x27 for a 16 chars and 2 line display
char sent_lcd_line[4][20];           // memory of what was sent to the LCD display

char buff_lcd_line[4][20];  // buffer in the lcd display

bool new_line;
int line_to_update;
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
String ProblemString;
unsigned long t_last_LCD = 0;

String command;
String command_in;
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

  for (int i_arr = 0; i_arr < 4; i_arr = i_arr + 1) {
    for (int j_arr = 0; j_arr < 20; j_arr = j_arr + 1) {
      buff_lcd_line[i_arr][j_arr] = 32;
      sent_lcd_line[i_arr][j_arr] = 32;
    }
  }

  Serial.begin(115200);
  delay(100);
  Serial1.begin(115200);
  delay(100);

Serial.println("LCD module initialize!");
Serial.flush();
  lcd.init();  // initialize the lcd
               // lcd.init();
  lcd.backlight();
  delay(100);
Serial.println("LCD module finish!");



  // Serial1.println("************************************");
  // Serial1.println("Serial UP");


  //pinMode(ledPin, OUTPUT);  // use the LED as an output

  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy module failed!");
    while (1);
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
    Serial.println("Bluetooth Command L");
    Serial.println(RandomProblem.value());
    command_received = true;
    command = "R" + String(RandomProblem.value());
  }

  if (Serial1.available()) {
    command_in = Serial1.readString();
    command_received = true;
  }

  if (Serial.available()) {
    command_in = Serial.readString();
    command_received = true;
  }


  // handle commands from the other arduino and pass these to the LCD display

  if (command_received) {
    // Serial.println(command);
    // delay(10);
    Serial1.println(command);

    if (command_in.startsWith("L")) {
      set_LCD_array();
    }
    //  Serial.flush();
    command_received = false;
  }
  // **************************************
  // check Bluetooth
  //*************************************
  BLE.poll();
  compare_LCD_array();
  if (new_line) {
    set_LCD();
  }
  blink_light();
}

void compare_LCD_array() {

  new_line = false;
  for (int i_arr = 0; i_arr < 4; i_arr = i_arr + 1) {
    for (int j_arr = 0; j_arr < 20; j_arr = j_arr + 1) {
      if (buff_lcd_line[i_arr][j_arr] != sent_lcd_line[i_arr][j_arr]) {
        new_line = true;
        line_to_update = i_arr;
        break;
      }
    }
    if (new_line) {
      break;
    }
  }
}

void set_LCD_array() {
  // change the value in the bufffer to the value recieved from serial
  char current_char;
  char lcd_line[20];
  int row_index;
  int imax;

  if (command_in.startsWith("L1")) {
    row_index = 0;
  } else if (command_in.startsWith("L2")) {
    row_index = 1;
  } else if (command_in.startsWith("L3")) {
    row_index = 2;
  } else if (command_in.startsWith("L4")) {
    row_index = 3;
  } else {
    row_index = 0;
  }


  imax = min(20, command_in.length() - 4);
  for (int i = 0; i <= 20; i = i + 1) {
    if (i < imax) {
      current_char = command_in.charAt(i + 2);
    } else {
      current_char = 32;
    }
    if (current_char > 31 && current_char < 127) {
      buff_lcd_line[row_index][i] = current_char;
    } else {
      buff_lcd_line[row_index][i] = 32;
    }
  }
}

void set_LCD() {

  char lcd_line_next[20];


  if (millis() > t_last_LCD + 800) {
    for (int i = 0; i <= 20; i = i + 1) {
      lcd_line_next[i] = buff_lcd_line[line_to_update][i];
    }
    t_last_LCD = millis();
    lcd.setCursor(0,line_to_update);
    delay(10);
    lcd.print(lcd_line_next);
    Serial.println(lcd_line_next);
    for (int i = 0; i <= 20; i = i + 1) {
      sent_lcd_line[line_to_update][i] = lcd_line_next[i];
    }
  }
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
