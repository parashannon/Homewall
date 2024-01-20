
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
BLEIntCharacteristic RandomProblem("005", BLERead | BLEWrite);      //fb

bool flip_problem = false;
int ProblemNumber;
String ProblemString;

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

  Serial.begin(115200);
  delay(100);
  Serial1.begin(115200);
  delay(100);

  lcd.init();  // initialize the lcd
 // lcd.init();
  lcd.backlight();


  Serial.println("************************************");
  Serial.println("Serial UP");

  Serial1.println("************************************");
  Serial1.println("Serial UP");

  for (int i_arr = 0; i_arr < 4; i_arr = i_arr + 1) {
    for (int j_arr = 0; j_arr < 20; j_arr = j_arr + 1) {
      buff_lcd_line[i_arr][j_arr] = 32;
      sent_lcd_line[i_arr][j_arr] = 32;
    }
  }


  pinMode(ledPin, OUTPUT);  // use the LED as an output

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
    //Serial1.println(command);

    if (command_in.startsWith("L")) {
      set_LCD();
    }
    //  Serial.flush();
    command_received = false;
  }
  // **************************************
  // check Bluetooth
  //*************************************
  BLE.poll();

  blink_light();
}

void set_LCD() {
  int command_length;
  int imax ;

  String lcd_line;
  lcd_line = "                    ";
  //        12345678901234567890
  if (command_in.startsWith("L1")) {
    lcd.setCursor(0, 0);
  } else if (command_in.startsWith("L2")) {
    lcd.setCursor(0, 1);
  } else if (command_in.startsWith("L3")) {
    lcd.setCursor(0, 2);
  } else if (command_in.startsWith("L4")) {
    lcd.setCursor(0, 3);
  }

  if (command_in.startsWith("L")) {
    imax = min(20, command_in.length() -4);

    for (int i = 0; i <= imax; i = i + 1) {
      lcd_line.setCharAt(i, command_in.charAt(i + 2));
    }
    lcd.print(lcd_line);
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
