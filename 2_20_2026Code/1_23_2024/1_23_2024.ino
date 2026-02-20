#include "arduino_secrets.h"
#include <stdint.h>
#include <FastLED.h>
#include "thingProperties.h"
#include "homewall_var.h"
#include "random_generation.h"
#include "other_functions.h"

struct HoldDecoded {
  int row;         // 1..16
  int col;         // 1..11 (after hard flag is stripped)
  int difficulty;  // 0 = easy use, 1 = hard use (based on "col>20" encoding)
  bool start_hold;
  bool end_hold;
  bool valid;      // false if entry==0 or row/col invalid
};


const int ledPin = LED_BUILTIN;  // set ledPin to on-board LED
unsigned long loop_count = 0;
unsigned int localPort = 2390;  // local port to listen on
unsigned long run_time_overflow = 0;
unsigned long rainbowtime = 0;
int Alexa_Column;  // column from Alexa
int Alexa_Row;     // row from Alexa
int led_mode = 0; // stores mode 0 - normal 1-rainbow
int random_mid;
int max_difficulty;
int n_rows = 16;
int n_columns = 11;
#define DATA_PIN 3  // pin sending data to leds
#define NUM_LEDS 400
CRGB leds[NUM_LEDS];
char command_string[256]; // holds output string to send to serial
char packetBuffer[256];               //buffer to hold incoming packet
char ReplyBuffer[] = "acknowledged";  // a string to send back
int diff_level; // difficulty level for random problems
int name_storage_index[4] = {0,0,0,0}; // contains the problem number for name storage overrides
char name_storage[4][20]; // contains names for name storage overrides
char temp_name[20];
//WiFiUDP Udp;
bool verbose_set;
bool flicker_mode = false;
bool first_flicker = true;
unsigned long flicker_time = 0;
int max_row = 4;
int max_column = 5;
int irow_temp;
int icolumn_temp;
bool flip_problem = false;
int ProblemNumber;
String ProblemString;
String command;
String TempString;
String P7String;
String P17String;
String P37String;
String P77String;
String two_words;
int LED_row_column_i;
bool command_received = false;
unsigned long t_update;
unsigned long t_current;
bool onboard_light = false;
bool first_loop = true;
bool cloud_enable = false;
unsigned long tstartdelay;
unsigned long t_LED_drifter_start = 0;
int total_diff = 0; // total problem difficulty
int min_good = 0; // minimum hold goodness
bool sent_serial = false;

void setup() {


  pinMode(ledPin, OUTPUT);  // use the LED as an output
  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
  setBlack();
  delay_loop(500);
  t_update = millis() + 1000;
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  delay_loop(100);
  Serial1.begin(115200);
  delay_loop(100);
  //Serial.println("Coded 1_5_2024");
  // Serial1.println("LX");
  delay(500);
  // Defined in thingProperties.h
  initProperties();

  // Connect to Arduino IoT Cloud
  ArduinoCloud.begin(ArduinoIoTPreferredConnection,false);

  /*
     The following function allows you to obtain more information
     related to the state of network and IoT Cloud connection and errors
     the higher number the more granular information you’ll get.
     The default is 0 (only errors).
     Maximum is 4
  */
  setDebugMessageLevel(0);
  ArduinoCloud.printDebugInfo();
  randomSeed(analogRead(0));
  //Serial.println("Setup Complete");
  //Serial.println(__DATE__);
}

void loop() {

  if (millis() > 15000 && !sent_serial) {
    Serial.println("Restarted, Date Compiled:");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);
    sent_serial = true;

  }
  if (cloud_enable) {
    ArduinoCloud.update();
  }
  // *************************************************
  // look for serial commands
  // *************************************************

  if (Serial.available()) {
    command = Serial.readString();
    //Serial.println("RX_" + command);
    if (command.startsWith(":")) {
      command_received = true;
    }
  }

  if (Serial1.available()) {
    command = Serial1.readString();
    //Serial.println("RX1_" + command);
    if (command.startsWith(":")) {
      command_received = true;
    }
  }


  if (command_received) {
    if (command.startsWith(":P")) {

      ProblemString = command.substring(2);
      ProblemNumber = ProblemString.toInt();
      if (ProblemNumber == 1 || ProblemNumber == 99) {
        t_LED_drifter_start = millis();
      } else if (ProblemNumber==2){
        t_LED_drifter_start = millis();
      } else {
        setProblem(true);
      }

    } else if (command.startsWith(":F")) {
      flip_problem = !flip_problem;
      setProblem(true);

    } else if (command.startsWith(":T")) {

      TempString = command.substring(2);
      LED_row_column_i = TempString.toInt();

      addRemove(LED_row_column_i);
    } else if (command.startsWith(":S")) {
      // print problem to serial 1
      serial1_print_problem();
    } else if (command.startsWith(":X")) {
    // set Problem N to vales d,d,d,d,
    // :XN-d,d,d,d,d,d,d,d,d,d,d,d,d,...x20
      TempString = command.substring(2);
      //Serial.println("recieved:"+TempString);
      add_and_set_problem(TempString);
    } else if (command.startsWith(":Q")) {
    // Query the Raspberry Pi for the string cccccccccccccccccccc
    // from :Qcccccccccccccccccccccccccccccccccc
      TempString = command.substring(2);

      // Copy characters from TempString to temp_name
    for (int i = 0; i < 20; i++) {
        if (i < TempString.length())
            temp_name[i] = TempString[i]; // Copy character from TempString
        else
            temp_name[i] = ' '; // Fill remaining characters with spaces
    }
    
      Serial.print("ilookup:");
      Serial.println(TempString);


    } else if (command.startsWith(":R")) {
      TempString = command.substring(2);
      random_mid = TempString.toInt();
      randomzie_problem();
    } else if (command.startsWith(":V")) {
      Serial.print("V ");
      Serial.print(__DATE__);
      Serial.print(" ");
      Serial.println(__TIME__);
    } else if (command.startsWith(":C")) {
      // toggle cloud enable
      cloud_enable=!cloud_enable;
      if (cloud_enable) {
        Serial.println("cloud on");
      } else {
        Serial.println("cloud off");
      }
    } else if (command.startsWith(":K")) {
      // reboot

      Serial.println("Rebooting, bye bye!");
      delay(100);
      if (millis()>10000){ // don't just get caught in a reboot cycle
        reboot_function();
      }
    } else {
      //Serial.println("Invalid command");
    }
  }

  if (led_mode == 1) {
    rainbow_up();
    FastLED.show();
  }

  if (ProblemNumber == 1 || ProblemNumber == 99) {
    LED_drifter();

  }

  if (ProblemNumber==2){
    many_moves();
  }

  command_received = false;
  loop_count = loop_count + 1;


  blink_light();
  if (millis() > 4000000000) {
    run_time_overflow = 1;
  }

  if (first_loop & millis() > 40000) {
    //internetStatus = "Up and Running";
    first_loop = false;
  }
}

void setBlack() {
  for (int iLED = 0; iLED < NUM_LEDS; iLED = iLED + 1) {
    leds[iLED] = CRGB::Black;
  }
  FastLED.show();
}

void reboot_function(){
  NVIC_SystemReset(); 
}

// rainbow the homewall, hopefully it doesn't start on fire
void rainbow_up() {
  byte i_color;
  byte start_color;
  byte colorshift;
  int t_loop = 5000;
  unsigned long t_current = 0;
  int LED_index;
  t_current = millis();

  colorshift = ((millis() % t_loop) * 255) / t_loop;

  for (int iLED = 0; iLED < 330; iLED = iLED + 1) {
    leds[iLED] = CRGB::Black;
  }


  if (t_current > rainbowtime) {
    rainbowtime = t_current + 500;
  }

  for (int irow = 0; irow < 15; irow = irow + 1) {
    start_color = 17 * irow;

    i_color = colorshift + start_color;
    for (int icolumn = 0; icolumn < 15; icolumn = icolumn + 1) {
      LED_index = LED_dict[irow][icolumn];
      leds[LED_index] = CHSV( i_color, 255, 200);

    }

  }


}

// turn on a problem from the problem library
void setProblem(bool verbose) {

  int LED_row_column;
  int LED_row;
  int LED_column;
  bool end_hold;
  bool start_hold;
  bool hard_hold;
  int LED_index;
  //Serial.print("Changing Problem to ");
  //Serial.println(ProblemNumber);
  // set everything to black



  if (ProblemNumber > 0 && ProblemNumber < 100 ) {
    led_mode = 0;
    for (int iLED = 0; iLED < NUM_LEDS; iLED = iLED + 1) {
      leds[iLED] = CRGB::Black;
    }
    // set each LED in the problem
    //
    for (int iLED = 0; iLED < 20; iLED = iLED + 1) {
      // Note that problem number starts with 1, array starts with 0

      LED_row_column = Problem_Library[ProblemNumber - 1][iLED];


      if (LED_row_column != 0) {
        end_hold = false;
        start_hold = false;
        hard_hold = false; // can't use the hold in the easy way
        // Serial.println(LED_row_column);
        // if the number was negative, this is a starthold
        // if the number was > 10000, this is an end hold
        if (LED_row_column < 0) {
          LED_row_column = abs(LED_row_column);
          start_hold = true;
        }

        if (LED_row_column > 10000) {
          LED_row_column = LED_row_column - 10000;
          end_hold = true;
        }

        LED_column = LED_row_column % 100;
        LED_row = (LED_row_column - LED_column) / 100;
        if (LED_column > 20) {
          LED_column = LED_column % 20;
          hard_hold = true;

        }
        if (flip_problem) {
          LED_column = 12 - LED_column;
        }
        if (LED_column > 0 && LED_column < 12 && LED_row < 17 && LED_row > 0) {
          setLight(LED_row, LED_column, end_hold, start_hold, hard_hold);
          // Serial.print(LED_row);
          // Serial.print(",");
          // Serial.print(LED_column);
          // Serial.print("|");
        } else {
          // Serial.println("Row/Column out of range");
        }
      }
    }
    if (flip_problem) {
      // problem is flipped, ie, "blue"
      leds[398] = CRGB( 0, 0, 100);
    } else {
      leds[398] = CRGB( 100, 0, 0);
    }
    byte difficulty_color;
    difficulty_color = 65 + 2 * ProblemNumber;
    leds[397] = CHSV( difficulty_color, 255, 255); // led is scaled based on proble number/difficulty
    FastLED.show();
    // Serial.print(":");
    // Serial.println("Set Complete");
    problem.setBrightness(ProblemNumber);
  } else if (ProblemNumber == 100) {
    led_mode = 1;
    problem.setBrightness(ProblemNumber);
  } else {
    // Serial.println("Invalid Problem Number");
  }
  if (verbose) {


    char temp_name_LCD[20]; 
    // check the name to see if this problem has a name override
    bool name_override=false;
    //Serial.print(ProblemNumber);
    //Serial.print("vs");
    for (int icheck=0; icheck < 4; icheck++){
      
     // Serial.print(name_storage_index[icheck]);
      //Serial.print("|");
      if (ProblemNumber == name_storage_index[icheck]){
        name_override=true;
        //Serial.print("Name Override");
        for (int ichar=0; ichar < 20; ichar++){
          temp_name_LCD[ichar]=name_storage[icheck][ichar];
        }
        break;
      }
    }
   // Serial.println(":Search Complete");
    
    //internetStatus = NameArray[ProblemNumber - 1]; //String example
    Serial1.println("l1Problem: " + String(ProblemNumber));
    Serial1.flush();
    delay(100);

    if (name_override){
      Serial1.println("l2" + String(temp_name_LCD));
      //Serial.println("found l2" + String(temp_name_LCD));
    } else {
      Serial1.println("l2" + String(NameArray[ProblemNumber - 1]));
    }
    
    Serial1.flush();
    delay(100);
    Serial1.println("l3" + String(CommentArray[ProblemNumber - 1]));
    Serial1.flush();
    delay(100);
    if (ProblemNumber == 7 ) {
      Serial1.println("L4L" + P7String);
      Serial.println("L4L" + P7String);
   } else if (ProblemNumber == 17) {
      Serial1.println("L4L" + P17String);
      Serial.println("L4L" + P17String);
   } else if (ProblemNumber == 37) {
      Serial1.println("L4L" + P37String);
      Serial.println("L4L" + P37String);
    } else if (ProblemNumber == 77) {
      Serial1.println("L4L" + P77String);
      Serial.println("L4L" + P77String);
    } else {
      if (ProblemNumber == 100) {
        Serial1.println("L4" + String(__DATE__) + String(__TIME__));
      } else {
        Serial1.println("L4                    ");
      }
    }

    Serial1.flush();

    Serial.print("Setting Problem:");
    Serial.print(ProblemNumber);
    Serial.print("   Side: ");
    Serial.println(flip_problem);
    serial_print_problem();
    serial1_print_problem();
  }

}

// set colors for light on a board
void set_LED_color(int LED_index, bool end_hold, bool start_hold, bool hard_hold) {

  if (LED_index < NUM_LEDS - 1 & LED_index >= 0) {
    if (start_hold) {
      leds[LED_index].green = 255;
      leds[LED_index].blue = 255;
      leds[LED_index].red = 0;
    } else if (end_hold) {
      leds[LED_index].green = 0;
      leds[LED_index].blue = 255;
      leds[LED_index].red = 255;
    } else if (hard_hold) {
      leds[LED_index].green = 150;
      leds[LED_index].blue = 0;
      leds[LED_index].red = 255;

    } else {
      leds[LED_index].green = 255;
      leds[LED_index].blue = 50;
      leds[LED_index].red = 100;
    }
  } else {
    // Serial.println("LED Index out of Range");
  }
}

void setLightToColor(int LED_row, int LED_column, byte Red, byte Green, byte Blue) {
  int LED_index;
  int LED_index_left;
  LED_index = LED_dict[LED_row - 1][LED_column - 1];

  if (LED_column > 1 & LED_column < 11) {
    // look at the light index to the left
    LED_index_left = LED_dict[LED_row - 1][LED_column - 2];
    if (LED_index_left > LED_index) {
      // the hold on the left is up one
      LED_index_left = LED_index + 1;
    } else {
      //the hold on the left is down one
      LED_index_left = LED_index - 1;
    }

    leds[LED_index_left].red = Red;
    leds[LED_index_left].green = Green;
    leds[LED_index_left].blue = Blue;
  }

  leds[LED_index].red = Red;
  leds[LED_index].green = Green;
  leds[LED_index].blue = Blue;

}

void showDifficulty() {
  int holddiff;
  byte R;
  byte G;
  byte B;
  for (int row_i = 1; row_i < 16; row_i = row_i + 1) {
    for (int column_i = 1; column_i < 12; column_i = column_i + 1) {
      holddiff = valid_holds[row_i][column_i];
      holddiff = holddiff % 10;
      //Serial.println(holddiff);
      if (holddiff == 5) {
        R = 0;
        G = 0;
        B = 200;
      } else if (holddiff == 4) {
        R = 0;
        G = 200;
        B = 60;
      } else if (holddiff == 3) {
        R = 180;
        G = 180;
        B = 0;
      } else if (holddiff == 2) {
        R = 230;
        G = 75;
        B = 0;
      } else if (holddiff == 1) {
        R = 254;
        G = 0;
        B = 0;
      } else {
        R = 0;
        G = 0;
        B = 0;
      }



      setLightToColor(row_i, column_i, R, G, B);


    }
  }
  FastLED.show();
}

//set an LED given row/column
void setLight(int LED_row, int LED_column, bool end_hold, bool start_hold, bool hard_hold) {
  int LED_index;
  int LED_index_left;
  LED_index = LED_dict[LED_row - 1][LED_column - 1];

  set_LED_color(LED_index, end_hold, start_hold, hard_hold);
  // we also need to set the led on the next hold
  if (LED_row < 16) {
    if (LED_column > 1 & LED_column < 11) {
      // look at the light index to the left
      LED_index_left = LED_dict[LED_row - 1][LED_column - 2];
      if (LED_index_left > LED_index) {
        // the hold on the left is up one
        LED_index_left = LED_index + 1;
      } else {
        //the hold on the left is down one
        LED_index_left = LED_index - 1;
      }
      set_LED_color(LED_index_left, end_hold, start_hold, hard_hold);
    }
  } else {
    // column 16, everything is special
    if (LED_column == 1 | LED_column == 11) {
      // set heelhook row

      for (int icolumn = 0; icolumn < 11; icolumn = icolumn + 1) {
        LED_index = LED_dict[0][icolumn];
        if (leds[LED_index] == 0) {
          leds[LED_index].red = 125;
          leds[LED_index].green = 60;
          leds[LED_index].blue = 40;
        }
      }
    } else if (LED_column == 2) { // left side
      for (int irow = 1; irow < 14; irow = irow + 1) {
        LED_index = LED_dict[irow][0];
        if (leds[LED_index] == 0) {
          leds[LED_index].red = 61;
          leds[LED_index].green = 30;
          leds[LED_index].blue = 20;
        }
      }
      } else if (LED_column == 4) { // top
      set_LED_color(338, end_hold, start_hold, hard_hold);
    } else if (LED_column == 5) { // top
      set_LED_color(336, end_hold, start_hold, hard_hold);
    } else if (LED_column == 6) { // kick board

      for (int icolumn = 3; icolumn < 8; icolumn = icolumn + 1) {
        LED_index = LED_dict[0][icolumn];
        if (leds[LED_index] == 0) {
          leds[LED_index].red = 250;
          leds[LED_index].green = 150;
          leds[LED_index].blue = 10;

          leds[LED_index - 1].red = 250;
          leds[LED_index - 1].green = 150;
          leds[LED_index - 1].blue = 10;
        }
      }

    } else if (LED_column == 7) { // rop right
      set_LED_color(332, end_hold, start_hold, hard_hold);
      } else if (LED_column == 8) { // rop right
      set_LED_color(330, end_hold, start_hold, hard_hold);
    } else if (LED_column == 10) { // right side
      for (int irow = 1; irow < 14; irow = irow + 1) {
        LED_index = LED_dict[irow][10];
        if (leds[LED_index] == 0) {
          leds[LED_index].red = 61;
          leds[LED_index].green = 30;
          leds[LED_index].blue = 20;
        }
      }

    }


  }

}

// adds or removes an LED from a boulder problem, then lights up the problem
void addRemove(int LED_row_column) {
  bool in_problem = false;
  int iLED_row_column;

  // check to see if the row column is in the problem
  for (int iLED = 0; iLED < 20; iLED = iLED + 1) {
    if (Problem_Library[ProblemNumber - 1][iLED] == LED_row_column) {
      in_problem = true;
      Problem_Library[ProblemNumber - 1][iLED] = 0;
    }
  }
  // First check to see if it is there but a different color, then change the color
  if (!in_problem) {
    for (int iLED = 0; iLED < 20; iLED = iLED + 1) {
      if (abs(Problem_Library[ProblemNumber - 1][iLED]) % 10000 == abs(LED_row_column) % 10000) {
        in_problem = true;
        Problem_Library[ProblemNumber - 1][iLED] = LED_row_column;
      }
    }
  }

  // if it wasn't in the problem add it in the first zero
  if (!in_problem) {
    for (int iLED = 0; iLED < 20; iLED = iLED + 1) {
      if (Problem_Library[ProblemNumber - 1][iLED] == 0) {
        Problem_Library[ProblemNumber - 1][iLED] = LED_row_column;
        break;
      }
    }
  }
  // show the problem
  setProblem(false);
}

// blink status lights
void blink_light() {
  t_current = millis();
  if ((t_current%1000 > 500) == onboard_light) {
    // if these are equal, change status and then flip status back
    // if they mismatch then do nothing
    //t_update = t_current + 500;
    if (onboard_light) {
      digitalWrite(LED_BUILTIN, HIGH);
      if (cloud_enable) {
      leds[399] = CRGB( 0, 0, 100);
      } else {
        leds[399] = CRGB( 60, 80, 60);
      }
    } else {
      digitalWrite(LED_BUILTIN, LOW);
      leds[399] = CRGB( 0, 0, 0);
    }
    
    FastLED.show();
    onboard_light = !onboard_light;
  }
}

// alexa update to problem
void onProblemChange() {
  // Add your code here to act upon Problem change
  uint8_t r, g, b, bright;
  problem.getValue().getRGB(r, g, b);
  //Serial.println("R:" + String(r) + " G:" + String(g) + " B:" + String(b));  //prints the current R, G, B values
  int alexa_brightness = max(max(r, g), b);
  int alexa_problem;
  alexa_problem = round((float)alexa_brightness / 2.55);

  command_received = true;
  command = ":P" + String(alexa_problem);
  //Serial.println("Alexa Command Recieved:");
  //Serial.println(command);

  if (r >= b) {
    flip_problem = false;
  } else {
    flip_problem = true;
  }
}

// alexa update to row
void onRowChange() {
  // Store the column from Alexa in an array, we won't use that until a row is given
  Alexa_Row = row.getBrightness();

  if (Alexa_Row == 99) {
    row.setBrightness(100);
    Serial.print("Showing Diff");
    showDifficulty();

  }
  //Serial.print("Alexa Row: ");
  //Serial.println(Alexa_Row);
}

// alexa update to column
void onColumnChange() {
  uint8_t r, g, b;
  // Add your code here to act upon Boulder change
  Alexa_Column = column.getBrightness();
  int Alexa_LED_row_column;
  // continue if row and column are valid
  if (Alexa_Column < 12 & Alexa_Row < 17) {
    column.getValue().getRGB(r, g, b);

    // there are three options, red, green, and white for end, start, normal
    //Serial.print("Alexa Row: ");
    //Serial.println(Alexa_Row);
    Alexa_LED_row_column = Alexa_Row * 100 + Alexa_Column;
    if (r > b) {
      Alexa_LED_row_column = Alexa_LED_row_column + 10000;  // end hold
    } else if (g > r) {
      Alexa_LED_row_column = -Alexa_LED_row_column;  // start hold
    }

    if (millis() > 45000 | run_time_overflow == 1) {  // ignore alexa row column commands for first 45 seconds
      command = ":T" + String(Alexa_LED_row_column);
      command_received = true;
      // Serial.print(millis());
      // Serial.print(":");
      // Serial.println(command);
    }
  }
}




/*
  Since Random is READ_WRITE variable, onRandomChange() is
  executed every time a new value is received from IoT Cloud.
*/

void onRandomProblemChange()  {
  // Add your code here to act upon Random change
  random_mid = randomProblem.getBrightness();
  if (loop_count > 500) { // ignore alexa row column commands for first few loops

    randomzie_problem();
  }
}

void ProblemtoRowColumn(int ArrayNum, int *Prow, int *Pcolumn) {
    *Pcolumn = (ArrayNum % 10000) % 100;
    *Prow = (ArrayNum % 10000 - *Pcolumn) / 100;
}

void serial_print_problem() {
  Serial.print("Problem: ");
  Serial.print(ProblemNumber);
  Serial.println(":");
  for (int i = 0; i < 20; i++) {

    Serial.print(Problem_Library[ProblemNumber - 1][i]);

    if (i < 19) {
      Serial.print(" , ");
    }

  }
  Serial.println("");
}

void serial1_print_problem() {
  Serial1.print("Problem: ");
  Serial1.print(ProblemNumber);
  Serial1.println(":");
  for (int i = 0; i < 20; i++) {
    if (Problem_Library[ProblemNumber - 1][i] != 0) {
      Serial1.print("|");
      Serial1.print(i);
      Serial1.print(" ");
      Serial1.print(Problem_Library[ProblemNumber - 1][i]);
    }
  }
  Serial1.println("");
  Serial.flush();
}


void delay_loop(unsigned long delay) {
  tstartdelay = millis();
  while (millis() < (tstartdelay + delay)) {
    //do nothing here
  }
}

void randomzie_problem() {

  if (random_mid % 10 == 7) {

    if (random_mid >  60) {
      ProblemNumber = 77;
      //min_good=1;
    } else if ( random_mid >  40)  {

      ProblemNumber = 37;
      // min_good=3;
    } else if ( random_mid >  20)  {

      ProblemNumber = 17;
      // min_good=3;
    }else {
      ProblemNumber = 7;
    }

    diff_level = random_mid / 10 + 1;

    two_words = "null berry pi";
    Serial.println("grw");
    Serial.flush();

    
    setaRandomProblem();

    for (int itry = 0; itry < 300; itry = itry + 1) {
      if (Serial.available()) {
        two_words = Serial.readString();
        //Serial.println("tw:" + two_words);
        break;
      }
      delay(20);

      if (itry == 299) {
        Serial.println("No tw Response");
      }
    }

    if (ProblemNumber == 7) {
      P7String = String(diff_level) + ":" + two_words;
    } else if (ProblemNumber == 17) {
      P17String = String(diff_level) + ":" + two_words;
    }else if (ProblemNumber == 37) {
      P37String = String(diff_level) + ":" + two_words;
    }else if (ProblemNumber == 77) {
      P77String = String(diff_level) + ":" + two_words;
    }

    setProblem(true);
  } else {
    // pick a random pre existing problem
    for (int iswap = 0; iswap < 4; iswap = iswap + 1) {


      for (int itry = 0; itry < 10; itry = itry + 1) {
        ProblemNumber = random(max(1, random_mid - 15), min(100, random_mid + 15));
        if (Problem_Library[ProblemNumber - 1][0] != 0) {
          break;
        }
      }
      if (iswap < 3) {
        setProblem(false);
      } else {
        setProblem(true);
      }
      delay_loop(500);
    }
  }
}


/*
  Since InternetStatus is READ_WRITE variable, onInternetStatusChange() is
  executed every time a new value is received from IoT Cloud.
*/
void onInternetStatusChange()  {
 //Add your code here to act upon InternetStatus change
}

void onSaveProblemChange()  {
 //Add your code here to act upon onSaveProblemChange change
}

void setaRandomProblem() {
  // This code creates a random problem
  int icolumn;
  int irow;
  int n_row_move;
  int n_column_move;
  int total_move_sq;
  //                        0   1     2   3     4    5   6       7     8     9     10
  int diff_per_level[11]={300, 300, 350, 471, 653,  940, 1700, 2600, 3500, 4300, 5000};
                       //{300, 300, 350, 471, 653,  920, 1600, 2400, 3500, 4200, 4900}; pre 9/25/2025
                       //{300, 300, 350, 471, 653,  920, 1600, 2400, 3500, 4600, 6300};                                 
                       //{300, 300, 350, 471, 653,  889, 1350, 1813, 2200, 2700, 3300};
                       //{300, 300, 350, 471, 653,  889, 1277, 1613, 1996, 2425, 2950}; Pre 5/7/2025
  int WH_per_level[11]=  {2,   2,    2,   2,    2,   1,   1,     1,     1,    1,    1};
                      // {300, 300, 350, 471, 653,  889, 1177, 1513, 1896, 2325, 2797}; Pre original
  
  bool valid_move;
  int icolumn_old;
  int irow_old;
  int ihold;
  int irand;
  bool valid_hold;
  bool heel_start = 0;
  bool kick_start = false;
  bool board_side;
  bool  isfeet = false;
  int worst_hold_allowable = 1;
  int ms_delay=10;
  bool heel_rail = false;
  total_diff = 0;

  max_row = 3 + ((diff_level) / 4);
  max_column = 4 + ((diff_level) / 4);
  max_difficulty = diff_per_level[diff_level];
  //= 300+pow((diff_level-1),1.78)*50;
    //300+(diff_level-1)*(diff_level-1)*40/sqrt(diff_level) ; // 220 + (diff_level - 1) * (diff_level - 1) * 60;

  worst_hold_allowable = WH_per_level[diff_level] ; //max(1, 4 - ((diff_level + 1) / 2));
  //Serial.println("Setting a Random Problem");
  Serial.print("Level: ");
  Serial.print(diff_level);
  Serial.print("  Max Diff: ");
  Serial.println(max_difficulty);
  Serial.println("iter|hold|nrow|ncol|diff|hrat|minh|info");

  board_side = random(1, 2 + 1) < 2;



  // set to zeros
  for (int iset = 0; iset < 20; iset = iset + 1) {
    Problem_Library[ProblemNumber - 1][iset] = 0;
  }
  // choose feet
  irand = int( random(1, 12 + 1));

  ihold = 0;
  if (irand < 10) {
    if (board_side) {
      Problem_Library[ProblemNumber - 1][ihold] = 101;
    } else {
      Problem_Library[ProblemNumber - 1][ihold] = 111;
    }
  } else if (irand < 11 && diff_level > 2) {

    Problem_Library[ProblemNumber - 1][ihold] = 1601;
    heel_start = 1;
  } else {
    Problem_Library[ProblemNumber - 1][ihold] = 1606;
    kick_start = true;
  }
  ihold = ihold + 1;

  //pick if the left side is on or not
  irand = int(random(1, 10 + 1));
  if (irand > 8) {
    heel_rail = true;
    if (board_side) {
      Problem_Library[ProblemNumber - 1][ihold] = 1602;
    } else {
      Problem_Library[ProblemNumber - 1][ihold] = 1610;
    }
    ihold = ihold + 1;

    // make things harder now
    //max_row++;
    //max_difficulty=max_difficulty+max_difficulty/2;

  }



  
  // pick a start hold on the first three rows

  if (kick_start) {
  //pick_hold(irow_old, icolumn_old,            min_row, last_hold_difficulty, worst_hold_allowable, max_row, max_column    max_allow_diff, last_direction                );
    pick_hold(2,           random(4,8+1),             -1,        3,            worst_hold_allowable,        1,          1,   min(max_difficulty,600),0, -1  );
    icolumn = icolumn_temp % 20;
    irow = irow_temp;
    
  } else if (heel_start) {
    pick_hold(1,           random(2,11),      0,        3,            worst_hold_allowable,        1,          2,   min(max_difficulty,600),0 , -2  );
    icolumn = icolumn_temp % 20;
    irow = irow_temp;
  } else {
    if (board_side) {
            //pick_hold(irow_old, icolumn_old, min_row, last_hold_difficulty, worst_hold_allowable, max_row, max_column    max_allow_diff, last_direction                );
            pick_hold(2,           random(2,5),    -1,        4,              worst_hold_allowable,       1,          2,   min(max_difficulty,600),   0 , -2 );
            icolumn = icolumn_temp % 20;
            irow = irow_temp;
    } else {
            //pick_hold(irow_old, icolumn_old, min_row, last_hold_difficulty, worst_hold_allowable, max_row, max_column    max_allow_diff, last_direction                );
            pick_hold(2,           random(8,11),   -1,        4,            worst_hold_allowable,        1,          2,   min(max_difficulty,600),   0, -2  );
            icolumn = icolumn_temp % 20;
            irow = irow_temp;
    }
  }


  Problem_Library[ProblemNumber - 1][ihold] = -1 * (100 * irow + icolumn);
  setProblem(false);
  delay(ms_delay);
  ihold = ihold + 1;
  icolumn_old = icolumn;
  irow_old = irow;


  int last_hold_difficulty;
  last_hold_difficulty = valid_holds[irow][icolumn] % 10 ;

  irand = int(random(1, 100 + 1));
  if (irand < (240 / (diff_level + 1) - 20)) {
    // pick a second start hold
    //void pick_hold( irow_old, icolumn_old, min_row, last_hold_difficulty, min_hold_level,  max_row_move,  max_column_move,  max_allow_diff, last_direction, min_column_move)
    pick_hold(irow_old, icolumn_old,   -1,        5,                    1,                  1,          2, min(max_difficulty,700),0,  -2 );

    icolumn = icolumn_temp % 20;
    irow = irow_temp;
    Problem_Library[ProblemNumber - 1][ihold] = -1 * (100 * irow + icolumn);
    setProblem(false);
    delay(ms_delay);
    ihold = ihold + 1;
    // icolumn_old = icolumn;
    //irow_old = irow;
  }

  irand = int(random(1, 100 + 1));
  if ((irand < (240 / (diff_level ) - 20) && irow_old > 1) || (irow_old > 2  && diff_level < 4 && !heel_rail)) {
    // pick any hold on the first row on this side of the board
    //Serial.println("Adding first row hold");
    if (board_side && !kick_start) {
      //pick_hold(irow_old, icolumn_old, min_row, last_hold_difficulty, worst_hold_allowable, max_row, max_column  );
      pick_hold(1,          3,             0,          5,                    1,                   0,       3, max_difficulty,0, -max_column );
    } else if( !kick_start) {
      pick_hold(1,          9,             0,          5,                    1,                   0,       3, max_difficulty ,0, -max_column );

    } else {
      pick_hold(1,          6,             0,          5,                    1,                   0,       3, max_difficulty ,0, -max_column );
    }
    icolumn = icolumn_temp % 20;
    irow = irow_temp;
    Problem_Library[ProblemNumber - 1][ihold] = (100 * irow + icolumn);
    setProblem(false);
    delay(ms_delay);
    ihold = ihold + 1;
    // icolumn_old = icolumn;
    // irow_old = irow;
  }

  // set the starting hold to somewhere near the feet so it doesn't just take off across the bottom
  if (!kick_start && max_difficulty < 1200){
    if (board_side){
      icolumn_old=3;
    } else {
      icolumn_old=9;
    }
  } 
  

  int last_direction=0;
  int last_delta_row=0;
  while (ihold < 20) {


    if (last_delta_row > 1 && random(0,800) > max_difficulty && random(0,700) > max_difficulty && irow_old>3 ){ 
      // pick a bonus hold
      //irow_old = irow;
      //pick_hold(irow_old, icolumn_old, min_row, last_hold_difficulty, worst_hold_allowable, max_row, max_column  );
      pick_hold(irow_old, icolumn_old,   -1,        5,                    1,                  0,          2, 1800,0, -max_column  );
      icolumn = icolumn_temp % 20;
      irow = irow_temp;
      last_delta_row=0;
      // do not reset _old
      
    } else {

      // check to see if there are feet, at all for the next move
      if  (irow_old> 5 && irow_old < 15){
   
        int numfeet;
        numfeet=max(holds_in_area(irow_old, icolumn_old, -6, 0, -5, 5), \
          holds_in_area(irow_old, icolumn_old, -7, 0, -4, 4));


        if (numfeet<2) {
             // pick a bonus random foot

              icolumn = random(max(icolumn_old-5,1),min(icolumn_old+5,11+1));
              irow = random(max(irow_old-6,1),irow_old-2+1);
              Problem_Library[ProblemNumber - 1][ihold] = (100 * irow + icolumn);
              ihold=ihold+1;
          Serial.print("EX FOOT:");
          Serial.print(numfeet);
          Serial.print("|");
          Serial.println(100 * irow + icolumn);
        }
        

      }


      

      int current_max_difficulty;

      int min_row = 0;
      // pick a normal hold
      if (heel_rail && ((board_side && icolumn_old < 6) || ( (!board_side && icolumn_old > 6) ))) {
        // there is a nearby heel rail, can be a bit harder
        current_max_difficulty=max_difficulty+ max_difficulty/3;
      } else {
        // pick a hold like normal
        current_max_difficulty=max_difficulty;
      }
  
      pick_hold(irow_old, icolumn_old, min_row, last_hold_difficulty, worst_hold_allowable, max_row, max_column, current_max_difficulty,last_direction,-max_column );
      
      last_direction=icolumn_temp%20-icolumn_old;
      last_delta_row=irow_temp-irow_old;
      
      icolumn = icolumn_temp % 20;
      irow = irow_temp;
  
  
      icolumn_old = icolumn;
      irow_old = irow;
  
  
      if (icolumn_temp > 20) {
        last_hold_difficulty = valid_holds[irow][icolumn]%100 / 10 ;
      } else {
        last_hold_difficulty = valid_holds[irow][icolumn] % 10 ;
      }
  

      
    }

   if (((irow > random(13,15)) && (last_hold_difficulty >= worst_hold_allowable) )|| (ihold == 19)) {
     // random 13,15 should drop probability of stopping on row 14 by ~50%
      Problem_Library[ProblemNumber - 1][ihold] = 10000 + 100 * irow + icolumn_temp;
      ihold = 20;
    } else {
      Problem_Library[ProblemNumber - 1][ihold] = 100 * irow + icolumn_temp;
    }

    
    ihold = ihold + 1;

    setProblem(false);
    delay(ms_delay);
  }



}

int holds_in_area(int ref_row, int ref_column, int row_minus, int row_plus, int column_minus, int column_plus){

  // returns the number of holds in an area
   int Pcolumn;
   int Prow;
  int holds;
  holds=0;
   for (int iset = 0; iset < 20; iset = iset + 1) {

        Pcolumn = (abs(Problem_Library[ProblemNumber - 1][iset]) % 10000) % 100;

     if (Pcolumn>20){
        Pcolumn=Pcolumn-20;
     }
        
        Prow = (abs(Problem_Library[ProblemNumber - 1][iset]) % 10000 - Pcolumn) / 100;
        if (Prow == 0) {
          break;
        }
        if (Prow == 16 && Pcolumn == 6) {
          // this is the kickboard
          Prow = -3;
        }

        if (Prow == 1 && (Pcolumn == 1 || Pcolumn == 11)) {
          // this is the kickboard
          Prow = -2;
        }

        if (((Pcolumn >= ref_column + column_minus) && (Pcolumn <= ref_column + column_plus)    ) && ((Prow >= ref_row + row_minus) && (Prow <= ref_row + row_plus) )) {
          holds =holds+1;
          // break;
        }
      }

  return holds;
}

//void pick_hold( irow_old, icolumn_old, min_row, last_hold_difficulty, min_hold_level,  max_row_move,  max_column_move,  max_allow_diff, last_direction, min_column_move) 


void pick_hold(int irow_old, int icolumn_old, int min_row, int last_hold_difficulty, int min_hold_level, int max_row_move, int max_column_move, int max_allow_diff, int last_direction, int min_column_move) {
  //finds a hold and sets icolumn_temp and irow_temp equal to the new position
  bool valid_hold = false;
  int irow;
  int icolumn;
  int n_row_move;
  int n_column_move;
  int total_move_sq;
  int randi;
  int max_iterations = 50;
  int itercount = 1;
  bool valid_move = true;
  int move_difficulty;
  int hold_rating;
  int hold_rating_raw;
  int last_hold_rating;
  bool harder_hold = false;
  bool isfeet = false;
  bool isundercling=false;
  bool wasfeet = false;
  bool wasmatch = false;
  char hold_info[4];
  int whitelist[300];
  int testint;
  bool old_holds=false;
  int  pick_count=0;
  int min_move_diff=0;
  
  itercount = 1;
// create a list of all valid moves and count them
   int l_column=min(max(min_column_move+icolumn_old,1),11); // leftmost column
   int r_column=max(min(max_column_move+icolumn_old,n_columns),1); // rightmost column
  
   int l_row=max(irow_old+min_row,1);
   int u_row=min(irow_old+max_row_move,n_rows);
   last_hold_rating = valid_holds[irow_old][icolumn_old];
if (last_hold_rating/1000 >0 ) {
  wasmatch=true;
}
  
    int i_whitelist=0;
    for (int clist = l_column; clist <= r_column; clist = clist + 1) {
            for (int rlist = l_row; rlist <= u_row; rlist = rlist + 1) {
              whitelist[i_whitelist]=rlist*100+clist;        
              i_whitelist=i_whitelist+1;
            }
    }
  
    int n_whitelist=i_whitelist-1;
  
  while (!valid_hold ) {

    hold_info[0]=' ';
    hold_info[1]=' ';
    hold_info[2]=' ';
    hold_info[3] = 0; // Explicitly set null terminator

    if (itercount> max_iterations/2) {
      max_row_move=max_row_move+1;
    }
    old_holds=true;
    pick_count=0;

    if (n_whitelist<1) {
      // reset the whitelist 
      //Serial.println("whitelist reset");
            int i_whitelist=0;
            u_row=min(irow_old+max_row_move,n_rows);
            for (int clist = l_column; clist <= r_column; clist = clist + 1) {
                for (int rlist = l_row; rlist <= u_row; rlist = rlist + 1) {
                whitelist[i_whitelist]=rlist*100+clist;
                i_whitelist=i_whitelist+1;
                }
            }
  
      n_whitelist=i_whitelist-1;
     }


      i_whitelist=random(0,n_whitelist+1); // remember this is exclusive, so max will be "n_whitelist"
      icolumn=whitelist[i_whitelist]%100;
      irow=(whitelist[i_whitelist]-icolumn)/100;
      if (i_whitelist<n_whitelist){
          whitelist[i_whitelist]=whitelist[n_whitelist]; // replace the spot on the whitelist with the last spot
      }

      n_whitelist=n_whitelist-1; // move the last spot in one
      //Serial.println(n_whitelist);



    
    valid_move = true;
    hold_rating_raw = valid_holds[irow][icolumn];
    
    harder_hold = false;
    if (hold_rating_raw % 100 > 10) {
      // the hold has two uses, roll for harder use
      randi = random(1, 10)+floor(max_allow_diff/900);
      if (randi > 5 && min_hold_level < 2) {
        hold_rating = (hold_rating_raw%100) / 10; // use the harder hold
        harder_hold = true;
        hold_info[0]='H';
      } else {
        hold_rating = hold_rating_raw % 10; // use the easier hold
        harder_hold = false;
      }
    } else {
      hold_rating = hold_rating_raw % 10;
    }

    // check to see if this is an undercling
    if (    ((hold_rating_raw/100)%10 == 1) ||   (((hold_rating_raw/100)%10 == 2) && (harder_hold == true ) )  ) { // the 100s digit is a 1 or a 2 for the         harder hold
      //Serial.print("Underclingr");
      //Serial.print(irow);
      //Serial.print(" c");
      //Serial.println(icolumn);
      isfeet = false;
      
      isfeet= (holds_in_area(irow, icolumn, -6, -3, -2, 2)>0)&&(holds_in_area(irow_old, icolumn_old, -5, -2, -3, 3)>0) ;
      isundercling=true;
      hold_info[1]='U';
      if (!isfeet) {
        valid_move = false;
        //Serial.println("No Feet");
      } else {
        // Serial.println("Feet Found!");
      }
    }

    
    n_row_move = (irow - irow_old);
    n_column_move = (icolumn - icolumn_old);
    int total_move_sq = (n_row_move * n_row_move*3)/2 + n_column_move * n_column_move;
    int iset_indx=0;
    // make sure the move isn't already in the climb
    for (int iset = 0; iset < 20; iset = iset + 1) {
      valid_move = (valid_move && !(abs(Problem_Library[ProblemNumber - 1][iset]) % 10000 == (irow * 100 + icolumn)));
      if (Problem_Library[ProblemNumber - 1][iset]==0){
        iset_indx=iset;
        break; // break and now iset is the location we are setting
      }
    }

    // get rid of holds that are below min hold level but loosen if we have made a lot of attempts
    if (( hold_rating  < min_hold_level) && (itercount < max_iterations/2)) {
      valid_move = false;
    } else if (((hold_rating  + 1) < min_hold_level ) && (itercount < (max_iterations + 50))) {
      valid_move = false; // try again with 1 diff worse holds
    }

    // reroll if there are too many holds on the same row
    int holds_on_row=holds_in_area(irow, icolumn, 0, 0, -11, 11);
    if (n_row_move == 0 && valid_move) {
      valid_move = (random(1, 10 + 1) > 5+3*(holds_on_row-1)) && valid_move; // reroll 50%/80%/100% of the time for 1/2/3 holds already on row
      // doesn't apply if we are moving back down to the row or up to a row with multiple holds (drifter mode)
    }
    
    // if the total move is greater than allowable max_row+1, 60% chance we reroll

    if (total_move_sq > pow(max(abs(max_row_move), abs(min_row)) + 1, 2) ) { // move is big
      valid_move = (random(1, 10 + 1) > 8) && valid_move;
    } else if  (total_move_sq <= 2) { //move is small, reroll
      valid_move = (random(1, 10 + 1) > 5) && valid_move;
    } else {
      valid_move = true && valid_move;
    }

    // get rid of easy holds IF you are on a harder difficulty

    if (max_allow_diff > 2100 ) {
      if (valid_holds[irow][icolumn] % 10  > random(1, 5 + 1)) { // can reroll 3, 4, 5
        valid_move = false;
      }

    } else if (max_allow_diff > 1000) {
      if (hold_rating  > random(3, 6 + 1)) { // can reroll 4, 5
        valid_move = false;
      }

    }

      isfeet = false;
      if (wasmatch){
      isfeet= (holds_in_area(irow, icolumn, -8, -2, -4, 4)>0) && (holds_in_area(irow, icolumn, -7, 0, -4, 4)>2) ;
      } else {
       isfeet= (holds_in_area(irow, icolumn, -8, -2, -4, 4)>0) && (holds_in_area(irow, icolumn, -7, 0, -4, 4)>3) ; 
      }
      wasfeet= (holds_in_area(irow_old, icolumn_old, -7, -2, -4, 4)>0);
      
    // holds_in_area(int ref_row, int ref_column, int row_minus, int row_plus, int column_minus, int column_plus)
    // the all important move difficulty formula. 
    move_difficulty = max(total_move_sq,2) * (pow((7 - last_hold_difficulty), 2) + pow((7 - hold_rating), 2))+ pow(5-hold_rating,2)*30-60;

    // check difficulty for prevous hold as well after the first two moves
    if (iset_indx>2 & max_allow_diff > 600) {
      HoldDecoded prev = parse_problem_entry(Problem_Library[ProblemNumber - 1][iset_indx-2]);
      int old_n_row_move = (irow - prev.row);
      int old_n_column_move = (icolumn - prev.col);
      int old_total_move_sq = (old_n_row_move * old_n_row_move*3)/2 + old_n_column_move * old_n_column_move;
      int old_move_difficulty=max(old_total_move_sq,2) * (pow((7 - prev.difficulty), 2) + pow((7 - hold_rating), 2))+ pow(5-hold_rating,2)*30-60;

      move_difficulty=min(move_difficulty,old_move_difficulty);
    }

if (!isfeet){
  move_difficulty=move_difficulty*2; // move is much harder if there are no feet
 
}

    if(isundercling){
      move_difficulty=(move_difficulty*4)/3;
    }

    if (!wasfeet){
 // move_difficulty=move_difficulty*3/2; // move is much harder if there are no feet
 
}

    
   // if this is a bump/cross it is harder
   if (last_direction*(n_column_move) > 0){
     move_difficulty=move_difficulty+(move_difficulty)/2+100;
      hold_info[2]='X';
   } else if (n_column_move==0 && n_row_move>2){
     move_difficulty=move_difficulty+100;
   }

   
    
    // 
    if (valid_move) {
     // if (max_allow_diff<2000){
      //  min_move_diff=((max_allow_diff*2 )/5 - 300);
      //} else {
      //  min_move_diff=((max_allow_diff*3 )/5 - 300);
      //}
      min_move_diff=max_allow_diff-(max_allow_diff+1400)/4;
      
      if ((move_difficulty > max_allow_diff || move_difficulty < min_move_diff)  && itercount < max_iterations) {
        valid_move = false;
      } else if (move_difficulty > max_allow_diff * 2  && itercount < max_iterations + 100) {
        valid_move = false;
      }
    }

    
    itercount = itercount + 1;
    valid_hold = ((hold_rating  > 0) && valid_move);
  }
  total_diff = max(total_diff, move_difficulty);

  Serial.print(itercount);
  Serial.print("| ");
  Serial.print(irow*100+icolumn);
  Serial.print("| ");
  Serial.print(n_row_move);
  Serial.print("| ");
  Serial.print(n_column_move);
  Serial.print("| ");
  Serial.print(move_difficulty);
  Serial.print("| ");
  Serial.print(hold_rating);
  Serial.print("/");
  Serial.print(min_hold_level);
  Serial.print("| ");
  Serial.println(hold_info);
  Serial.flush();
  
  if (harder_hold) {
    icolumn_temp = icolumn + 20;
  } else {
    icolumn_temp = icolumn;
  }
  irow_temp = irow;

}

void LED_drifter() {
  long t_drifter;
  static long t_next;
  long t_pause = 4000;
  static bool first_frame = true;
  static int ihold;
  static int last_hold_difficulty;
  static int icolumn_old;
  static int irow_old;
  int min_row;
  int icolumn;
  int irow;
  int min_hold_level = 1;
  int is_foot = 0;
  static int move_count;
  int n_keep = 8; // technically the index, so 5 is 6 holds, 0-5
  t_drifter = millis() - t_LED_drifter_start;
  bool just_started=true;
  if (t_drifter < 6000) {
    t_next = 6000;
    just_started=true;
  }
  if (ProblemNumber == 1) {
    n_keep = 8;
    t_pause = 4000;
  } else if (ProblemNumber == 99) {
    n_keep = 4;
    t_pause = 3000;

  }

  if ((t_drifter < 6000) && (first_frame)) {
    t_next = 6000;
    first_frame = false;

    // setup original problem here
    // set to zeros
    for (int iset = 0; iset < 20; iset = iset + 1) {
      Problem_Library[ProblemNumber - 1][iset] = 0;
    }
    // set start holds
    Problem_Library[ProblemNumber - 1][0] = -101;
    Problem_Library[ProblemNumber - 1][1] = 204;


    if (ProblemNumber == 99) {
      Problem_Library[ProblemNumber - 1][n_keep + 3] = -105;
      Problem_Library[ProblemNumber - 1][n_keep + 4] = -107;
      Problem_Library[ProblemNumber - 1][n_keep + 5] = -406;
      Problem_Library[ProblemNumber - 1][n_keep + 6] = -701;
      Problem_Library[ProblemNumber - 1][n_keep + 7] = -711;
      Problem_Library[ProblemNumber - 1][n_keep + 8] = -904;
      Problem_Library[ProblemNumber - 1][n_keep + 9] = -908;
      Problem_Library[ProblemNumber - 1][n_keep + 10] = -101;
      Problem_Library[ProblemNumber - 1][n_keep + 11] = -111;
    }


    last_hold_difficulty = valid_holds[2][4] % 10 ;
    icolumn_old = 4;
    irow_old = 2;
    ihold = 2;
    setProblem(true);
    move_count = 1;
    is_foot = 0;
  }

  if (t_drifter > t_next) {
    t_next = t_next + t_pause;
    first_frame = true;


    if (ihold < n_keep) {
      ihold = ihold + 1; // move to next position
    } else {
      //holds are full, shift everything left
      for (int iset = 0; iset < ihold; iset = iset + 1) {
        Problem_Library[ProblemNumber - 1][iset] = Problem_Library[ProblemNumber - 1][iset + 1];
      }
    }

    if (ihold > n_keep - 1) { // make second to last hold red
      Problem_Library[ProblemNumber - 1][1] = abs(Problem_Library[ProblemNumber - 1][1]) % 10000 + 10000;
    }

    if (ihold > n_keep - 2) { // make second to last hold red
      Problem_Library[ProblemNumber - 1][0] = abs(Problem_Library[ProblemNumber - 1][0]) % 10000 + 10000;
    }


    // if we pick a foot, it doesn't count as a hold wrt difficulty and last column/row.

    if ((irow_old > 3)  && (move_count % 4 == 0) && (ProblemNumber == 1)) {
      // add a new foot but don't update old row and column

      min_row = -5;
      max_row = -4;
      max_difficulty = 6000;
      //pick_hold(int irow_old, int icolumn_old, int min_row, int last_hold_difficulty, int min_hold_level, int max_row_move, int max_column_move)
      pick_hold( irow_old, icolumn_old, min_row,  5, 2, max_row, max_column, max_difficulty,0 , -max_column );
      icolumn = icolumn_temp % 20;
      irow = irow_temp;
      Problem_Library[ProblemNumber - 1][ihold] = -1 * (100 * irow + icolumn);
      is_foot = 1;
    } else {

      is_foot = 0;
      if (irow_old > 10) {
        //you can stop just moving up
        just_started=false;
      };
      // add a new hand
      if (just_started==true) {
        min_row = 1;
        max_row = 3;
      } else {
        if (irow_old > 11) {
          min_row = -3;
          max_row = 1;
        } else if (irow_old < 5) {
          min_row = -1;
          max_row = 3;
        } else {
          min_row = -3;
          max_row = 3;
        }

      }

      if (move_count < 15) {
        min_hold_level = 3;
        max_difficulty = 600;
      } else if (move_count < 30) {
        min_hold_level = 1;
        max_difficulty = 900;
      } else {
        min_hold_level = 1;
        max_difficulty = 3000;

      }


      pick_hold( irow_old, icolumn_old, min_row,  last_hold_difficulty, min_hold_level, max_row, max_column, max_difficulty,0, -max_column  );
      icolumn = icolumn_temp % 20;
      irow = irow_temp;

      icolumn_old = icolumn;
      irow_old = irow;

      if (icolumn_temp < 20) {
        last_hold_difficulty = valid_holds[irow][icolumn] % 10 ;
      } else {
        last_hold_difficulty = valid_holds[irow][icolumn] / 10 ;
      }
      Problem_Library[ProblemNumber - 1][ihold] = 100 * irow + icolumn_temp;
    }





    setProblem(false);
    delay(200);
    Serial1.println("L4Move: " + String(move_count) );
    Serial1.flush();
    move_count = move_count + 1;

  }

} // end of function


void add_and_set_problem(String problemString) {
  // Temporary storage for parsed integers
  int problems[20];
  int tempNumber = 0;
  boolean isNegative = false;
  int index = 0; // Index for filling the problems array


  for (int i = 0; i < 20; i++) {
    problems[i]=0;
  }
  
  // Parse the string into integers
  for(unsigned int i = 0; i < problemString.length(); i++) {
    if (problemString[i] == '-') {
      isNegative = true;
    } else if (problemString[i] >= '0' && problemString[i] <= '9') {
      tempNumber = tempNumber * 10 + (problemString[i] - '0');
    } else if (problemString[i] == ',' || i == problemString.length() - 1) {
      if (isNegative) {
        tempNumber *= -1;
      }
      problems[index++] = tempNumber; // Store the number
      // Reset for the next number
      tempNumber = 0;
      isNegative = false;
    }
  }

  // Determine the row to update
  int rowToUpdate = ProblemNumber - 1; // Assuming ProblemNumber is 1-indexed
  boolean isRowEmpty = true;
  for (int i = 0; i < 20; i++) {
    if (Problem_Library[rowToUpdate][i] != 0) {
      isRowEmpty = false;
      break;
    }
  }

  if (!isRowEmpty) {
    // If the row is not empty, set rowToUpdate to 97
    rowToUpdate = 97;
    ProblemNumber=98;
  }

  // Update the Problem_Library row with parsed integers
  //Serial.print("Setting Problem:");
  for (int i = 0; i < 20; i++) {
    Problem_Library[rowToUpdate][i] = problems[i];
   // Serial.print(problems[i]);
   // Serial.print(',');
    
  }
  //Serial.println("");

  // add a line in temp names for this (new or overwrite)
  for (int i = 0; i < 4; i++) {
    if (name_storage_index[i]==0 || name_storage_index[i]==ProblemNumber){
      name_storage_index[i]=ProblemNumber;
      //Serial.print("indx:");
      //Serial.print(name_storage_index[i]);
      //Serial.print("chars:");
      for (int ichar = 0; ichar < 20; ichar++) {
        name_storage[i][ichar]=temp_name[ichar];
        //Serial.print(name_storage[i][ichar]);
      }
      //Serial.println();
      break; // we are good, leave for loop
    }
  }
  

  setProblem(true);
}

void many_moves(){

  long t_drifter;
  long t_pause = 2500;
  static long t_next;

    int LED_row_column;
  int LED_row;
  int LED_column;
  bool end_hold;
  bool start_hold;
  int n_holds_side=13;  
  static bool first_frame = true;
  int imove;
  int const SequenceHands[13] ={105, 204, 401, 404, 503, 701, 704, 903, 1006, 1203, 1306, 1503, 1605  };

  static int move_count;
  //int n_keep = 8; // technically the index, so 5 is 6 holds, 0-5

  
  t_drifter = millis() - t_LED_drifter_start;

  if (t_drifter< t_pause && first_frame) {
    move_count=1;
    first_frame=false;
    t_next=t_pause;
    problem.setBrightness(ProblemNumber);

       for (int iLED = 0; iLED < NUM_LEDS; iLED = iLED + 1) {
      leds[iLED] = CRGB::Black;
    }
        Serial1.println("l1Problem: " + String(ProblemNumber));
    Serial1.flush();
    delay(100);
    Serial1.println("l2" + String(NameArray[ProblemNumber - 1]));
    Serial1.flush();
    delay(100);
    Serial1.println("l3" + String(CommentArray[ProblemNumber - 1]));
    Serial1.flush();

        Serial.print("Setting Problem:");
    Serial.print(ProblemNumber);
    Serial.flush();

    for (int ihold=0; ihold<n_holds_side; ihold++){
      LED_row_column=SequenceHands[ihold];
      LED_column = LED_row_column % 100;
      LED_row = (LED_row_column - LED_column) / 100;
      end_hold=false;
      start_hold=false;
      setLight(LED_row, LED_column, end_hold, start_hold, false);
      setLight(LED_row, 12-LED_column, end_hold, start_hold, false);
    }
    FastLED.show();
  }

  // start by setting up
  if (t_drifter > t_next) {
    first_frame=true;
    t_next = t_next + t_pause;

    // flush the board
    for (int iLED = 0; iLED < NUM_LEDS; iLED = iLED + 1) {
      leds[iLED] = CRGB::Black;
    }
    leds[397] = CHSV( 10, 255, 255); // led is scaled based on proble number/difficulty



    // change all the holds to green

    for (int ihold=0; ihold<n_holds_side; ihold++){
      LED_row_column=SequenceHands[ihold];
      LED_column = LED_row_column % 100;
      LED_row = (LED_row_column - LED_column) / 100;
      end_hold=false;
      start_hold=false;
      setLight(LED_row, LED_column, end_hold, start_hold, false);
      setLight(LED_row, 12-LED_column, end_hold, start_hold, false);
    }
    // we want three holds on at any given time, set these to blue. There are nholds -2 moves per side

      
    for (int ihold=move_count; ihold<move_count+4; ihold++){
      imove=ihold%((n_holds_side-3)*4);
      if (imove==0){
        imove=(n_holds_side-2)*4;
      }
      if (imove<=(n_holds_side-2)){
        // this is on the right side going up
        LED_row_column=SequenceHands[imove+2-1];
        LED_column = LED_row_column % 100;
        LED_row = (LED_row_column - LED_column) / 100;
        
      } else if (imove<=(n_holds_side-2)*2) {
        LED_row_column=SequenceHands[n_holds_side-(imove+2-n_holds_side)];
        LED_column = LED_row_column % 100;
        LED_row = (LED_row_column - LED_column) / 100;
        LED_column=12-LED_column;
      } else if (imove<=(n_holds_side-2)*3) {
                LED_row_column=SequenceHands[imove+2-1-(2*(n_holds_side-2))];
        LED_column = LED_row_column % 100;
        LED_row = (LED_row_column - LED_column) / 100;
        LED_column=12-LED_column;
        
      } else {
        LED_row_column=SequenceHands[(n_holds_side-2)*3-(imove-n_holds_side)];
        LED_column = LED_row_column % 100;
        LED_row = (LED_row_column - LED_column) / 100;
      }
      if (ihold==move_count){
        start_hold=false;
        end_hold=true;
      } else {
        start_hold=true;
        end_hold=false;
      }
      
      setLight(LED_row, LED_column, end_hold, start_hold, false);
      
    }
    

    Serial1.println("L4Move: " + String(move_count) );
    Serial1.flush();
    FastLED.show();
    move_count=move_count+1;
  }
  

  
}

bool isInList(int target, int list[], int size) {
    for (int i = 0; i < size; i++) {
        if (list[i] == target) {
            return true;
            break;
        }
    }
    return false;
}
// -------------------------------
// How Problem_Library encodes holds
// -------------------------------
// Each problem is 20 integers: Problem_Library[problemIndex][0..19].
// Each entry encodes row/column plus flags in a single int:
//
// Base encoding (when "normal"):
//   base = row*100 + col
//
// Flags layered on top of base:
//   - Start hold: stored as NEGATIVE. (entry < 0)
//   - End hold: add 10000. (entry > 10000)
//   - Hard-use variant: add 20 to the column (so col becomes > 20).
//       In setProblem(): if (col > 20) { col = col % 20; hard_hold=true; }
//
// So a full stored value looks like:
//
//   entry = sign * ( (end?10000:0) + row*100 + (hard? (col+20) : col) )
//
// Parsing in setProblem does:
//   if negative -> start_hold=true, abs()
//   if >10000 -> end_hold=true, subtract 10000
//   col = entry % 100
//   row = (entry - col)/100
//   if (col > 20) { col = col % 20; hard_hold=true; }
//
// -------------------------------
// Parser helper
// -------------------------------


HoldDecoded parse_problem_entry(int entry_raw) {
  HoldDecoded h;
  
  h.row = 0; h.col = 0; h.difficulty = 0;
  h.start_hold = false; h.end_hold = false;
  h.valid = false;

  if (entry_raw == 0) return h;

  int entry = entry_raw;

  if (entry < 0) {
    h.start_hold = true;
    entry = abs(entry);
  }

  if (entry > 10000) {
    h.end_hold = true;
    entry -= 10000;
  }

  int col_raw = entry % 100;
  int row = (entry - col_raw) / 100;
  bool harder_hold=false;

  // hard-use encoding (column bumped by +20)
  if (col_raw > 20) {
    // h.difficulty = 1;         // hard use
    harder_hold=true;
    col_raw = col_raw % 20;   // recover real column
  } else {
    // h.difficulty = 0;         // easy use
  }

  h.row = row;
  h.col = col_raw;

  int hold_rating_raw = valid_holds[h.row][h.col];
 if (harder_hold){
          h.difficulty = (hold_rating_raw%100) / 10; // use the harder hold
      } else {
        h.difficulty = hold_rating_raw % 10; // use the easier hold
 }
  // basic sanity (match your setProblem bounds)
  if (h.col > 0 && h.col < 12 && h.row > 0 && h.row < 17) h.valid = true;

  return h;
}