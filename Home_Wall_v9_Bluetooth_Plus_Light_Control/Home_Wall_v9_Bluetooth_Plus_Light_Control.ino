#include <FastLED.h>
#include <ArduinoBLE.h>


const int ledPin = LED_BUILTIN;  // set ledPin to on-board LED
unsigned long loop_count = 0;
unsigned int localPort = 2390;  // local port to listen on
unsigned long run_time_overflow = 0;
unsigned long rainbowtime = 0;

int max_packet = 256;
bool new_line;
int line_to_update;
int command_from = 0;
const int buttonPin = 4;  // set buttonPin to digital pin 4



int led_mode = 1;  // stores mode 0 - normal 1-rainbow
int random_mid;
int max_difficulty;
int n_rows = 16;
int n_columns = 11;
#define DATA_PIN 3  // pin sending data to leds
#define NUM_LEDS 400
CRGB leds[NUM_LEDS];
char command_string[256];  // holds output string to send to serial
char command_in[256];      // commands coming in from serial

char packetBuffer[256];               //buffer to hold incoming packet
char ReplyBuffer[] = "acknowledged";  // a string to send back

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

BLEService WallServe("112");  // create service

// create switch characteristic and allow remote device to read and write

BLEIntCharacteristic Problem_Number("001", BLERead | BLEWrite);  //840b
BLELongCharacteristic ToggleLED("002'", BLERead | BLEWrite);     // 13D
BLELongCharacteristic FlipProblem("003", BLERead | BLEWrite);    //fb
BLEIntCharacteristic SaveProblem("004", BLERead | BLEWrite);     //fb
BLEIntCharacteristic RandomProblem("005", BLERead | BLEWrite);   //fb

unsigned long t_last_LCD = 0;

String command;
String TempString;
int LED_row_column_i;
bool command_received = false;

int const valid_holds[17][12] = {
  //  1  2  3  4  5  6  7  8  9  0  1
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  //X
  { 0, 0, 1, 0, 3, 3, 1, 3, 3, 0, 1, 0 },  //1
  { 0, 0, 2, 0, 5, 3, 1, 3, 5, 0, 2, 0 },  //2
  { 0, 0, 0, 3, 1, 1, 4, 1, 1, 3, 0, 0 },  //3
  { 0, 2, 1, 1, 4, 2, 2, 2, 4, 1, 1, 2 },  //4
  { 0, 1, 1, 2, 1, 3, 1, 3, 1, 2, 1, 1 },  //5
  { 0, 2, 2, 2, 2, 1, 1, 2, 2, 2, 2, 2 },  //6
  { 0, 5, 2, 3, 4, 1, 4, 1, 3, 3, 2, 5 },  //7
  { 0, 1, 2, 1, 3, 1, 2, 1, 3, 1, 2, 1 },  //8
  { 0, 2, 2, 3, 2, 1, 1, 1, 2, 3, 2, 2 },  //9
  { 0, 2, 1, 1, 2, 1, 4, 1, 2, 1, 1, 2 },  //10
  { 0, 5, 2, 4, 1, 2, 2, 2, 1, 4, 2, 5 },  //11
  { 0, 2, 4, 3, 1, 2, 1, 2, 1, 3, 4, 2 },  //12
  { 0, 2, 2, 1, 1, 2, 3, 2, 1, 1, 2, 2 },  //13
  { 0, 0, 1, 1, 1, 1, 2, 1, 1, 1, 1, 0 },  //14
  { 0, 0, 0, 2, 3, 1, 2, 1, 3, 2, 0, 0 },  //15
  { 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 0 },  //16
};

int LED_dict[16][11] = {
  // this translates board LED numbers
  { 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 19 },
  { 40, 38, 36, 34, 32, 30, 28, 26, 24, 22, 21 },
  { 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 62 },
  { 84, 82, 80, 78, 76, 74, 72, 70, 68, 66, 65 },
  { 87, 89, 91, 93, 95, 97, 99, 101, 103, 105, 106 },
  { 128, 126, 124, 122, 120, 118, 116, 114, 112, 110, 109 },
  { 131, 133, 135, 137, 139, 141, 143, 145, 147, 149, 150 },
  { 172, 170, 168, 166, 164, 162, 160, 158, 156, 154, 153 },
  { 175, 177, 179, 181, 183, 185, 187, 189, 191, 193, 194 },
  { 216, 214, 212, 210, 208, 206, 204, 202, 200, 198, 197 },
  { 219, 221, 223, 225, 227, 229, 231, 233, 235, 237, 238 },
  { 260, 258, 256, 254, 252, 250, 248, 246, 244, 242, 241 },
  { 263, 265, 267, 269, 271, 273, 275, 277, 279, 281, 282 },
  { 304, 302, 300, 298, 296, 294, 292, 290, 288, 286, 285 },
  { 307, 309, 311, 313, 315, 317, 319, 321, 323, 325, 326 },
  { 390, 390, 390, 390, 390, 390, 390, 390, 390, 390, 390 }  // this last row should never be used directly and is handled
  // differently. Note LED's 398 and 399 are used as status lights.
};
// column 16 is  [ lower edge, left side, nothing, 0, 0 ,0, 0, 0, 0, right side,  lower edge (redundant)
//                   1              2          3       4  5   6 7  8  9     10           11

//************************************************************************************
// this array stores every climb
// if a number is negative, it is a start hold. If a number has a 1 in the 10000 digit, it is
// end hold
//int Random_Storage [20]={	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	};
int Problem_Library[100][20] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P1	Ind _0	_	Null Problem
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P2	Ind_1	_	Pink is Lava
  { -105, -204, 406, 605, 701, 706, 904, 1006, 1306, 11605, 101, 0, 0, 0, 0, 0, 0, 0, 0, 0 },      //P3	Ind_2	_	Abby for Effort
  { 101, -105, -204, 404, 505, 704, 706, 1006, 701, 1306, 11503, 0, 0, 0, 0, 0, 0, 0, 0, 0 },      //P4	Ind_3	_	Gentle Giraffe
  { 111, -107, -204, 505, 1602, 805, 902, 1006, 1103, 1306, 11503, 602, 0, 0, 0, 0, 0, 0, 0, 0 },  //P5	Ind_4	_	Beggini Bear
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P6	Ind_5	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P7	Ind_6	_	Good Luck
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P8	Ind_7	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P9	Ind_8	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P10	Ind_9	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P11	Ind_10	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P12	Ind_11	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P13	Ind_12	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P14	Ind_13	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P15	Ind_14	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P16	Ind_15	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P17	Ind_16	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P18	Ind_17	_
  { 101, -104, -105, 404, 706, 903, 1006, 1306, 11508, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },          //P19	Ind_18	_	Half Orange
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P20	Ind_19	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P21	Ind_20	_
  { -105, 1601, 1602, 404, 405, 703, 704, 1004, 1202, 1406, 11605, 0, 0, 0, 0, 0, 0, 0, 0, 0 },    //P22	Ind_21	_	Blue Square
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P23	Ind_22	_
  { 101, -202, -306, 605, 607, 701, 808, 1008, 1210, 1406, 11607, 1602, 0, 0, 0, 0, 0, 0, 0, 0 },  //P24	Ind_23	_	Happy Birthday
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P25	Ind_24	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P26	Ind_25	_
  { 1601, -105, 303, 503, 703, 903, 1006, 1306, 11506, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },          //P27	Ind_26	_	Green
  { -202, 503, 703, 1004, 1306, 11506, 101, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },               //P28	Ind_27	_	Yellow
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P29	Ind_28	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P30	Ind_29	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P31	Ind_30	_
  { 1602, -602, 805, 1002, 1306, 11607, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                //P32	Ind_31	_	Toature
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P33	Ind_32	_
  { -303, 107, 401, 703, 1006, 1309, 11503, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },               //P34	Ind_33	_	Half Green
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P35	Ind_34	_
  { 111, -108, 406, 607, 1006, 1308, 11506, 1610, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },            //P36	Ind_35	_	Green Circle
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P37	Ind_36	_
  { 101, -104, -105, 404, 802, 907, 1104, 1208, 11504, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },          //P38	Ind_37	_	Half Pink
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P39	Ind_38	_
  { 101, -401, 601, 904, 1105, 11507, 1208, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },               //P40	Ind_39	_	SS Rhino
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P41	Ind_40	_
  { 101, -107, 406, 607, 1004, 1006, 1202, 11506, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },            //P42	Ind_41	_	Brown Square
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P43	Ind_42	_
  { -106, 406, 605, 808, 1008, 1202, 11506, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },               //P44	Ind_43	_	Pink
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P45	Ind_44	_
  { 101, -102, -304, 603, 904, 1104, 1206, 1404, 11605, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },         //P46	Ind_45	_	Half Yellow
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P47	Ind_46	_
  { 101, -105, -106, 406, 605, 607, 907, 1303, 11503, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },           //P48	Ind_47	_	Gritty Teeth
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P49	Ind_48	_
  { 111, -106, 304, 607, 903, 1206, 1404, 11605, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },             //P50	Ind_49	_	Orange
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P51	Ind_50	_
  { 101, -105, 406, 601, 806, 1003, 11506, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                //P52	Ind_51	_	Purple
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P53	Ind_52	_
  { 111, -408, 610, 1005, 1205, 11503, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                 //P54	Ind_53	_	Fantastic Deer
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P55	Ind_54	_
  { 101, -202, 401, 706, 905, 1206, 11506, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                //P56	Ind_55	_	Blue
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P57	Ind_56	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P58	Ind_57	_
  { -108, 111, 101, 105, 404, 706, 1004, 1006, 1008, 1306, 11605, 503, 0, 0, 0, 0, 0, 0, 0, 0 },   //P59	Ind_58	_	Whirly Dirly
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P60	Ind_59	_
  { 101, -104, 403, 607, 902, 1106, 1302, 11605, 1602, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },          //P61	Ind_60	_	Good Job Koala
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P62	Ind_61	_
  { 111, -210, 507, 806, 1602, 1001, 1402, 11506, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },            //P63	Ind_62	_	Pink Circle
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P64	Ind_63	_
  { 101, -303, 407, 804, 1102, 1207, 11507, 604, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },             //P65	Ind_64	_	Wonderful Hippo
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P66	Ind_65	_
  { 101, -401, 603, 806, 1208, 11503, 908, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                //P67	Ind_66	_	Eric's Climb
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P68	Ind_67	_
  { 101, -202, 604, 802, 1007, -1405, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                  //P69	Ind_68	_	Fantastic Work Bee
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P70	Ind_69	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P71	Ind_70	_
  { 101, -104, 504, 803, 1104, 1205, 1408, 11607, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },            //P72	Ind_71	_	Yellow Circle
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P73	Ind_72	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P74	Ind_73	_
  { 111, -108, 409, 910, 806, 904, 1004, 11405, -210, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },           //P75	Ind_74	_	Keep it Up Kitten
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P76	Ind_75	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P77	Ind_76	_	Gooder Luck
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P78	Ind_77	_
  { 101, -102, -304, 705, 707, 904, 1208, 1404, 1403, 11501, 11401, 0, 0, 0, 0, 0, 0, 0, 0, 0 },   //P79	Ind_78	_	Half Red
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P80	Ind_79	_
  { 111, -106, 405, 502, 1602, 904, 1109, 1408, -11508, -11509, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },    //P81	Ind_80	_	Erics Other Climb
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P82	Ind_81	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P83	Ind_82	_
  { 101, 108, -305, 702, 805, 1106, 11506, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                //P84	Ind_83	_	Green Bandit
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P85	Ind_84	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P86	Ind_85	_
  { 101, -106, -210, 509, 605, 909, 1004, 1210, 1506, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },           //P87	Ind_86	_	Red
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P88	Ind_87	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P89	Ind_88	_
  { 111, -104, 508, 606, 908, 1104, 1204, 11503, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },             //P90	Ind_89	_	Half Purple
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P91	Ind_90	_
  { 101, -102, -305, 808, 1602, 1308, 11509, 906, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },            //P92	Ind_91	_	Half Blue
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P93	Ind_92	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P94	Ind_93	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P95	Ind_94	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P96	Ind_95	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P97	Ind_96	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P98	Ind_97	_
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P99	Ind_98	_	The Endurance
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                  //P100	Ind_99	_	Rainbow Party
};                                                                                                 //P100	Ind_99	_	Rainbow Party};	//P100	Ind_99	_	Rainbow Party

const char* NameArray[] = {
  "Pink is Lava",
  "",
  "Abby for Effort",
  "Gentle Giraffe",
  "Beggini Bear",
  "",
  "Good Luck",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "Half Orange",
  "",
  "",
  "Blue Square",
  "",
  "Happy Birthday",
  "",
  "",
  "Green",
  "Yellow",
  "",
  "",
  "",
  "Toature",
  "",
  "Half Green",
  "",
  "Green Circle",
  "",
  "Half Pink",
  "",
  "SS Rhino",
  "",
  "Brown Square",
  "",
  "Pink",
  "",
  "Half Yellow",
  "",
  "Gritty Teeth",
  "",
  "Orange",
  "",
  "Purple",
  "",
  "Fantastic Deer",
  "",
  "Blue",
  "",
  "",
  "Whirly Dirly",
  "",
  "Good Job Koala",
  "",
  "Pink Circle",
  "",
  "Wonderful Hippo",
  "",
  "Eric's Climb",
  "",
  "Fantastic Work Bee",
  "",
  "",
  "Yellow Circle",
  "",
  "",
  "Keep it Up Kitten",
  "",
  "Gooder Luck",
  "",
  "Half Red",
  "",
  "Erics Other Climb",
  "",
  "",
  "Green Bandit",
  "",
  "",
  "Red",
  "",
  "",
  "Half Purple",
  "",
  "Half Blue",
  "",
  "",
  "",
  "",
  "",
  "",
  "The Endurance",
  "Rainbow Party",
};


const char* CommentArray[] = {
  "Get off the Lava",
  "",
  "Start Here",
  "So Gentle So Giraffe",
  "Still a Bear",
  "",
  "Roll the Dice",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "OG Warmup",
  "",
  "",
  "Heels 4 Fun",
  "",
  "Gift Heels",
  "",
  "",
  "Weird Start",
  "Do it again!",
  "",
  "",
  "",
  "Dev's Climb",
  "",
  "Local Classic",
  "",
  "Ariel's Climb",
  "",
  "Easy >> Harder",
  "",
  "Sher's Climb",
  "",
  "OG Not Classic",
  "",
  "I hate it.",
  "",
  "Crimp Ladder",
  "",
  "OG",
  "",
  "Into the Business",
  "",
  "Wheee!",
  "",
  "No Comment",
  "",
  "OG Hard",
  "",
  "",
  "It's an Experience!",
  "",
  "Good Job?",
  "",
  "Get that Toe",
  "",
  "",
  "",
  "Left of Pinch",
  "",
  "",
  "",
  "",
  "Don't Dab Wall",
  "",
  "",
  "Damn Last Move",
  "",
  "Meaner Dice",
  "",
  "OG Proj",
  "",
  "Thanks, Eric",
  "",
  "",
  "The Proj",
  "",
  "",
  "v2 in my gym",
  "",
  "",
  "All Emile",
  "",
  "2Morpho4I",
  "",
  "",
  "",
  "",
  "",
  "",
  "One More Move!",
  "Witness Pretty",

};



unsigned long t_update;
unsigned long t_current;
bool onboard_light = false;
bool first_loop = true;
unsigned long tstartdelay;
unsigned long t_LED_drifter_start = 0;


// Bluetooth stuff

void setup() {


  pinMode(ledPin, OUTPUT);  // use the LED as an output

  FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
  setBlack();

  delay(1000);

  t_update = millis() + 1000;
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  delay_loop(100);
  Serial1.begin(115200);
  delay_loop(100);


  randomSeed(analogRead(0));


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
  // add the service
  BLE.addService(WallServe);

  FlipProblem.writeValue(0);
  BLE.setLocalName("HomeWall");
  BLE.setDeviceName("HomeWall");
 
  BLE.advertise();

  Serial.println("Bluetooth® device active, waiting for connections...");
  Serial.println("Awaiting your command");
  
}

void loop() {
// look for bluetooth commands

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

  // *************************************************
  // look for serial commands
  // *************************************************

  if (Serial.available()) {
    command = Serial.readString();
    command_received = true;
  }

  if (Serial1.available()) {
    command = Serial1.readString();
    command_received = true;
  }


  if (command_received) {
    if (command.startsWith("P")) {

      ProblemString = command.substring(1);
      ProblemNumber = ProblemString.toInt();
      if (ProblemNumber == 1 || ProblemNumber == 99) {
        t_LED_drifter_start = millis();
      } else {
        setProblem(true);
      }

    } else if (command.startsWith("F")) {
      flip_problem = !flip_problem;
      setProblem(true);

    } else if (command.startsWith("T")) {

      TempString = command.substring(1);
      LED_row_column_i = TempString.toInt();

      addRemove(LED_row_column_i);
    } else if (command.startsWith("S")) {
      // Serial.print("Saving Problem Library to Memory");
      //Save_Problem_Library();
    } else if (command.startsWith("R")) {
      TempString = command.substring(1);
      random_mid = TempString.toInt();
      randomzie_problem();
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

  command_received = false;
  loop_count = loop_count + 1;
  
  // **************************************
  // check Bluetooth
  //*************************************
  BLE.poll();

  blink_light();
  if (millis() > 4000000000) {
    run_time_overflow = 1;
  }

  if (first_loop & millis() > 40000) {

    first_loop = false;
  }
}

void setBlack() {
  for (int iLED = 0; iLED < NUM_LEDS; iLED = iLED + 1) {
    leds[iLED] = CRGB::Black;
  }
  FastLED.show();
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
      leds[LED_index] = CHSV(i_color, 255, 200);
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
  int LED_index;
  //Serial.print("Changing Problem to ");
  //Serial.println(ProblemNumber);
  // set everything to black

  if (verbose) {

    Serial1.println("l1Problem: " + String(ProblemNumber));
    Serial1.flush();
    delay(500);
    Serial1.println("l2" + String(NameArray[ProblemNumber - 1]));
    Serial1.flush();
    delay(500);
    Serial1.println("l3" + String(CommentArray[ProblemNumber - 1]));
    Serial1.flush();
    delay(500);
    Serial1.println("L4                                          ");
  }
  Serial1.flush();

  if (ProblemNumber > 0 && ProblemNumber < 100) {
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

        if (flip_problem) {
          LED_column = 12 - LED_column;
        }
        if (LED_column > 0 && LED_column < 12 && LED_row < 17 && LED_row > 0) {
          setLight(LED_row, LED_column, end_hold, start_hold);

        } else {
        }
      }
    }
    if (flip_problem) {
      // problem is flipped, ie, "blue"
      leds[398] = CRGB(0, 0, 100);
    } else {
      leds[398] = CRGB(100, 0, 0);
    }
    byte difficulty_color;
    difficulty_color = 65 + 2 * ProblemNumber;
    leds[397] = CHSV(difficulty_color, 255, 255);  // led is scaled based on proble number/difficulty
    FastLED.show();
    // Serial.print(":");
    // Serial.println("Set Complete");
  } else if (ProblemNumber == 100) {
    led_mode = 1;
  } else {
    // Serial.println("Invalid Problem Number");
  }
}

// set colors for light on a board
void set_LED_color(int LED_index, bool end_hold, bool start_hold) {

  if (LED_index < NUM_LEDS - 1 & LED_index >= 0) {
    if (start_hold) {
      leds[LED_index].green = 255;
      leds[LED_index].blue = 255;
      leds[LED_index].red = 0;
    } else if (end_hold) {
      leds[LED_index].green = 0;
      leds[LED_index].blue = 255;
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

//set an LED given row/column
void setLight(int LED_row, int LED_column, bool end_hold, bool start_hold) {
  int LED_index;
  int LED_index_left;
  LED_index = LED_dict[LED_row - 1][LED_column - 1];

  set_LED_color(LED_index, end_hold, start_hold);
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
      set_LED_color(LED_index_left, end_hold, start_hold);
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
    } else if (LED_column == 2) {  // left side
      for (int irow = 1; irow < 14; irow = irow + 1) {
        LED_index = LED_dict[irow][0];
        if (leds[LED_index] == 0) {
          leds[LED_index].red = 61;
          leds[LED_index].green = 30;
          leds[LED_index].blue = 20;
        }
      }
    } else if (LED_column == 5) {  // top
      set_LED_color(336, end_hold, start_hold);
    } else if (LED_column == 7) {  // rop right
      set_LED_color(330, end_hold, start_hold);
    } else if (LED_column == 10) {  // right side
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
  if (t_current > t_update) {
    if (onboard_light) {
      digitalWrite(LED_BUILTIN, HIGH);
      leds[399] = CRGB(0, 0, 100);
    } else {
      digitalWrite(LED_BUILTIN, LOW);
      leds[399] = CRGB(0, 0, 0);
    }
    t_update = t_current + 1000;
    FastLED.show();
    onboard_light = !onboard_light;
  }
}


void randomzie_problem() {

  if (random_mid == 77) {
    max_row = 4;
    max_column = 5;
    ProblemNumber = 77;
    max_difficulty = 3000;
    setaRandomProblem();

    setProblem(true);
  } else if (random_mid == 7) {
    max_row = 3;
    max_column = 4;
    ProblemNumber = 7;
    max_difficulty = 650;
    setaRandomProblem();

    setProblem(true);
  } else {

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

void setaRandomProblem() {
  // This code creates a random problem


  int icolumn;
  int irow;

  int n_row_move;
  int n_column_move;
  int total_move_sq;
  bool valid_move;

  int icolumn_old;
  int irow_old;
  int ihold;
  bool valid_hold;
  // valid HAND holds
  // 1 - 5, 5 is a giant jugg, 1 is bad or undercling

  Serial.println("Setting a Random Problem");

  // set to zeros
  for (int iset = 0; iset < 20; iset = iset + 1) {
    Problem_Library[ProblemNumber - 1][iset] = 0;
  }
  // choose feet
  irow = int(random(1, 10 + 1));

  ihold = 0;
  if (irow < 9) {
    Problem_Library[ProblemNumber - 1][ihold] = 101;
  } else {
    Problem_Library[ProblemNumber - 1][ihold] = 1601;
  }
  ihold = ihold + 1;

  //pick if the left side is on or not
  irow = int(random(1, 10 + 1));
  if (irow > 8) {
    Problem_Library[ProblemNumber - 1][ihold] = 1602;
    ihold = ihold + 1;
  }


  irow = int(random(1, 10 + 1));

  // pick a random start hold on the first three rows
  valid_hold = false;
  while (!valid_hold) {
    irow = int(random(1, 3 + 1));
    icolumn = int(random(1, 6 + 1));
    valid_hold = valid_holds[irow][icolumn] > 0;
  }
  Problem_Library[ProblemNumber - 1][ihold] = -1 * (100 * irow + icolumn);

  // choose the remaining holds
  ihold = ihold + 1;
  icolumn_old = icolumn;
  irow_old = irow;
  int last_hold_difficulty;
  last_hold_difficulty = valid_holds[irow][icolumn];


  //
  while (ihold < 20) {

    int min_row = 0;
    pick_hold(irow_old, icolumn_old, min_row, last_hold_difficulty, 1);
    icolumn = icolumn_temp;
    irow = irow_temp;

    icolumn_old = icolumn;
    irow_old = irow;
    last_hold_difficulty = valid_holds[irow][icolumn];

    if (irow > 14) {

      Problem_Library[ProblemNumber - 1][ihold] = 10000 + 100 * irow + icolumn;

      ihold = 20;
    } else {
      Problem_Library[ProblemNumber - 1][ihold] = 100 * irow + icolumn;
    }
    ihold = ihold + 1;
  }
  serial_print_problem();
}

void pick_hold(int irow_old, int icolumn_old, int min_row, int last_hold_difficulty, int max_hold_difficulty) {
  //finds a hold and sets icolumn_temp and irow_temp equal to the new position
  bool valid_hold = false;
  int irow;
  int icolumn;
  int n_row_move;
  int n_column_move;
  int total_move_sq;
  int max_iterations = 300;
  int itercount = 1;
  bool valid_move = true;
  int move_difficulty;

  itercount = 1;
  while (!valid_hold || (irow_old == irow && icolumn_old == icolumn)) {
    irow = max(min(irow_old + int(random(min_row, max_row + 1)), n_rows), 1);                  // min_row is zero for normal sets
    icolumn = max(min(icolumn_old + int(random(-max_column, max_column + 1)), n_columns), 1);  // just stay in bounds
    n_row_move = (irow - irow_old);
    n_column_move = (icolumn - icolumn_old);
    int total_move_sq = n_row_move ^ 2 + n_column_move ^ 2;
    valid_move = true;
    // make sure the move isn't already in the climb
    for (int iset = 0; iset < 20; iset = iset + 1) {
      valid_move = (valid_move && !(abs(Problem_Library[ProblemNumber - 1][iset]) % 10000 == (irow * 100 + icolumn)));
    }

    if ((max_hold_difficulty > valid_holds[irow][icolumn]) && itercount < max_iterations) {
      valid_move = false;
    }

    // if the total move is greater than allowable max_row+1, 60% chance we reroll

    if ((total_move_sq > (max(abs(max_row), abs(min_row)) + 1) ^ 2)) {  // move is big
      valid_move = (random(0, 10) > 6) && valid_move;
    } else if (total_move_sq < 2) {  //move is small
      valid_move = (random(0, 10) > 8) && valid_move;
    } else {
      valid_move = true && valid_move;
    }
    move_difficulty = total_move_sq * ((6 - last_hold_difficulty) + (6 - valid_holds[irow][icolumn])) ^ 2;
    // 5 x 4 holds on 1 and 1 is about 4100. 4x1 holds x 3 difficulty is 612
    if (valid_move) {
      if (move_difficulty > max_difficulty && itercount < max_iterations) {
        valid_move = (random(1, 10) > 9);  // 10% chance to keep hard move, else, reroll
      }
    }
    itercount = itercount + 1;
    valid_hold = ((valid_holds[irow][icolumn] > 0) && valid_move);
  }
  icolumn_temp = icolumn;
  irow_temp = irow;
}

void serial_print_problem() {
  Serial.print("Problem: ");
  Serial.print(ProblemNumber);
  Serial.println(":");
  for (int i = 0; i < 20; i++) {
    Serial.print(i);
    Serial.print("|");
    Serial.println(Problem_Library[ProblemNumber - 1][i]);
  }
}

void delay_loop(unsigned long delay) {
  tstartdelay = millis();
  while (millis() < (tstartdelay + delay)) {
    //do nothing here
  }
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
  int max_hold_difficulty = 1;
  int is_foot = 0;
  static int move_count;
  int n_keep = 8;  // technically the index, so 5 is 6 holds, 0-5
  t_drifter = millis() - t_LED_drifter_start;

  if (t_drifter < 6000) {
    t_next = 6000;
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


    // roll the dice for left column
    if (int(random(1, 10)) > 8) {
      Problem_Library[ProblemNumber - 1][n_keep + 1] = 1602;
      Problem_Library[ProblemNumber - 1][n_keep + 2] = 1610;
    }

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


    last_hold_difficulty = valid_holds[2][4];
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
      ihold = ihold + 1;  // move to next position
    } else {
      //holds are full, shift everything left
      for (int iset = 0; iset < ihold; iset = iset + 1) {
        Problem_Library[ProblemNumber - 1][iset] = Problem_Library[ProblemNumber - 1][iset + 1];
      }
    }

    if (ihold > n_keep - 1) {  // make second to last hold red
      Problem_Library[ProblemNumber - 1][1] = abs(Problem_Library[ProblemNumber - 1][1]) % 10000 + 10000;
    }

    if (ihold > n_keep - 2) {  // make second to last hold red
      Problem_Library[ProblemNumber - 1][0] = abs(Problem_Library[ProblemNumber - 1][0]) % 10000 + 10000;
    }


    // if we pick a foot, it doesn't count as a hold wrt difficulty and last column/row.

    if ((irow_old > 3) && (move_count % 4 == 0) && (ProblemNumber == 1)) {
      // add a new foot but don't update old row and column

      min_row = -5;
      max_row = -4;
      max_difficulty = 3000;
      pick_hold(irow_old, icolumn_old, min_row, 5, 2);
      icolumn = icolumn_temp;
      irow = irow_temp;
      Problem_Library[ProblemNumber - 1][ihold] = -1 * (100 * irow + icolumn);
      is_foot = 1;
    } else {

      is_foot = 0;
      // add a new hand
      if (move_count < 4) {
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
        max_hold_difficulty = 3;
        max_difficulty = 500;
      } else if (move_count < 30) {
        max_hold_difficulty = 1;
        max_difficulty = 800;
      } else {
        max_hold_difficulty = 1;
        max_difficulty = 4000;
      }


      pick_hold(irow_old, icolumn_old, min_row, last_hold_difficulty, max_hold_difficulty);
      icolumn = icolumn_temp;
      irow = irow_temp;

      icolumn_old = icolumn;
      irow_old = irow;
      last_hold_difficulty = valid_holds[irow][icolumn];
      Problem_Library[ProblemNumber - 1][ihold] = 100 * irow + icolumn;
    }





    setProblem(false);
    delay(200);
    Serial1.println("L4Move: " + String(move_count));
    Serial1.flush();
    move_count = move_count + 1;
  }

}  // end of function
