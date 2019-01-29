/*
    INTRO:
    a simple golf solitaire game, written for nokia 5110 display and a 3.3v pro mini controller, by Echo Liu
    version 0.1
    2019.1.27

   PIN LAYOUT:
   ------------------
   pro mini       5110
   -------------------
   pin 9          CLK
   pin 8          CE
   pin 7          DIN
   pin 6          DC
   pin 5          RST
   pin 4          VCC
   GND            GND
   VCC            BL
   -------------------
   --------------------
   pro mini       OTHER
   --------------------
   pin A0         pro mini RST
   pin 10-13      4 buttonS ----> GND
   --------------------

   NOTE:
   1. display part of the code is written from ground up to save some flash space, but it is not that neccesary
   2. software SPI is used, due to the fact it is a card game, SPI speed is not of first priority
   3. interrupt based debouncing is used, with alright performance, but not perfect, code is based on http://www.ganssle.com/debouncing-pt2.htm
   4. EEPROM is used just for storing high scores.
   5. a software reset is used
   6. part of the Amstrad CPC extended font is recreated, from https://fontstruct.com/fontstructions/show/25590/amstrad_cpc_extended
   7. codes of EEPROM reading and writing int type data is from https://www.instructables.com/id/two-ways-to-reset-arduino-in-software/
   8. number might be a bit small but to accomadate the small display, a 2 pixel width number font is created
   9. Sketch uses 5420 bytes (17%) of program storage space. Maximum is 30720 bytes.
      Global variables use 409 bytes (19%) of dynamic memory, leaving 1639 bytes for local variables. Maximum is 2048 bytes.

*/

#include <EEPROM.h>
//display pins
const uint8_t DIN = 7;
const uint8_t CLK  = 9;
const uint8_t CE = 8;
const uint8_t RST = 5;
const uint8_t DC = 6;
const uint8_t VCC = 4;
const uint8_t RESET = A0;

//initial deck array
uint8_t DECK[] = {1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 13};
//number display array for lcd. nPart is for card number, NumForDisplay is for remaining card number and score. ArrowDisplay is for drawing arrows.
const uint8_t nPart[13][4] = {{0x81, 0x81, 0xBD, 0x81}/* 1 */, {0x81, 0xB5, 0xAD, 0x81}/* 2 */, {0x81, 0xB5, 0xBD, 0x81}/* 3 */, {0x81, 0x8D, 0xBD, 0x81}/* 4 */, {0x81, 0xAD, 0xB5, 0x81}/* 5 */, {0x81, 0xBD, 0xB5, 0x81}/* 6 */, {0x81, 0x85, 0xBD, 0x81}/* 7 */, {0x81, 0xBD, 0xB9, 0x81}/* 8 */, {0x81, 0xAD, 0xBD, 0x81}/* 9 */, {0xBD, 0x81, 0xBD, 0xBD}/* 10 */, {0xA5, 0xA5, 0xBD, 0x85}/* J */, {0xBD, 0xA5, 0xA5, 0x9D}/* Q */, {0x81, 0xBD, 0x99, 0xA5}/* K */};
const uint8_t NumForDisplay[10][2] = {{B00111100, B00111100}/*0*/, {B00000000, B00111100}/*1*/, {B00110100, B00101100}/*2*/, {B00110100, B00111100}/*3*/, {B00001100, B00111100}/*4*/, {B00101100, B00110100}/*5*/, {B00111100, B00110100}/*6*/, {B00000100, B00111100}/*7*/, {B00111100, B00111000}/*8*/, {B00101100, B111100}/*9*/};
const uint8_t ArrowDisplay[3] = { B00010000, B00011000, B00011100};
const uint8_t AlphaDisplay[27][7] = {/*A0*/{B00000000, B11111000, B11111100, B00100110, B00100110, B11111100, B11111100},/*B*/{B10000010, B11111110, B11111110, B10010010, B10010010, B11111110, B01101100},
                                           /*C2*/{B00111000, B01111100, B11000110, B10000010, B10000010, B11000110, B01000100},/*D*/{B10000010, B11111110, B11111110, B10000010, B11000110, B01111100, B00111000},
                                           /*E4*/{B10000010, B11111110, B11111110, B10010010, B10111010, B10000010, B11000110},/*F*/{B10000010, B11111110, B11111110, B10010010, B00111010, B00000010, B00000110},
                                           /*G6*/{B01111000, B11111100, B10000110, B10000010, B10100010, B11100110, B11100100},/*H*/{B00000000, B11111110, B11111110, B00010000, B00010000, B11111110, B11111110},
                                           /*I8*/{B00000000, B10000010, B10000010, B11111110, B11111110, B10000010, B10000010},/*J*/{B01100000, B11100000, B10000000, B10000010, B11111110, B01111110, B00000010},
                                           /*K10*/{B10000010, B11111110, B11111110, B00010000, B00111000, B11101110, B11000110},/*L*/{B10000010, B11111110, B11111110, B10000010, B10000000, B11000000, B11100000},
                                           /*M12*/{B11111110, B11111110, B00011100, B00111000, B00011100, B11111110, B11111110},/*N*/{B11111110, B11111110, B00001100, B00011000, B00110000, B11111110, B11111110},
                                           /*O14*/{B00111000, B01111100, B11000110, B10000010, B11000110, B01111100, B00111000},/*P*/{B10000010, B11111110, B11111110, B10010010, B00010010, B00001110, B00001100},
                                           /*Q16*/{B01111000, B11111100, B10000110, B10100010, B01100110, B11011100, B10111000},/*R*/{B10000010, B11111110, B11111110, B00010010, B00110010, B01111110, B11001100},
                                           /*S18*/{B00000000, B01001100, B11011110, B10010010, B10010010, B11110110, B01100100},/*T*/{B00000000, B00000110, B10000010, B11111110, B11111110, B10000010, B00000110},
                                           /*U20*/{B00000000, B01111110, B11111110, B10000000, B10000000, B11111110, B01111110},/*V*/{B00000000, B00111110, B01111110, B11000000, B11000000, B01111110, B00111110},
                                           /*W22*/{B11111110, B11111110, B01100000, B00110000, B01100000, B11111110, B11111110},/*X*/{B11000010, B11100110, B00111100, B00011000, B00111100, B11100110, B11000010},
                                           /*Y24*/{B00000000, B00001110, B10011110, B11110000, B11110000, B10011110, B00001110},/*Z*/{B10001110, B11000110, B11100010, B10110010, B10011010, B11001110, B11100110},
                                           /* 26*/{B00000000, B00000000, B00000000, B00000000, B00000000, B00000000, B00000000}
                                    };
// Last position for arrows, used for erasing arrows, col, row
uint8_t ArrowLastPosition[2] = {7, 5};
// button pressed state detection, to avoid the situation when the button is pressed for a bit longer, multiple pressed msg will be generated.
uint8_t PastPressed = B00000000; //only last 4 bit will be used to store if the 4 buttons, this byte is to record button press event last time checked.
uint8_t ButtonState = B00000000; //PastPressed is used to record wheater button is pressed before. ButtonState is going to be used later in the main loop logic.
// CONTROL AND GAME LOGIC PART
uint8_t CardStatues[7] = { 5, 5, 5, 5, 5, 5, 5}; // in order to track the number of cards on each col, for arrow moving and deck array card tracking as well
uint8_t LastRevealed = 35; // the array no. of the revealed card. at the beginning of the game DECK[35] will be revealed by default
uint8_t RemainingCards = 16; // no. of remaining cards, for display, for locating next card to be revealed and to end the game when this no. turns zero
uint8_t Seeked = 0;
unsigned long TempTime; // for scores
int Score = 0;
uint8_t CP = 0; // "CONSECTIVE PLAY" for socres
uint8_t SS1[4] = {6, 14, 11, 5}; // GOLF
uint8_t SS2[9] = {18, 14, 11, 8, 19, 0, 8, 17, 4}; //SOLITAIRE
uint8_t SS3[8] = {4, 2, 7, 14, 26, 11, 8, 20}; // ECHO LIU
uint8_t ED1[9] = {6, 0, 12, 4, 26, 14, 21, 4, 17}; // GAME OVER
uint8_t ED2[10] = {24, 14, 20, 17, 26, 18, 2, 14, 17, 4}; // YOUR SCORE
uint8_t ED3[10] = {7, 8, 6, 7, 26, 18, 2, 14, 17, 4}; // HIGH SCORE

//button debouncing part
uint8_t RawKeyStatues() {
  return PINB;
}

//#define MAX_CHECKS 10;
volatile uint8_t DebouncedResults = 0;
uint8_t State[10];
uint8_t INDEX = 0;

ISR(TIMER2_OVF_vect) {
  // timer2 overflow interrupt function
  uint8_t i, j;
  State[INDEX] = RawKeyStatues();
  //every time this interrupt triggers, the State array got updated
  INDEX++;
  j = 0xFF; //11111111
  for (i = 0; i < 10; i++) j = j & State[i];
  //this is the most important part of this debouncing timer interrup. if, and only if all corresponding bit of State[] are 1, the resualting j will be 1. one 0 j will be 0.
  //when button is pressed, after bouncing 10 interrupt will all be 1 for one pin, then that pin will put 1 to j bit.
  //when button is released, during bouncing 1 interrup come up with a 0 for one pin, the j bit will be 0. but nothing will happen for button release so this does not matter in this case at all.
  //but this requires all button to be ACTIVE HIGH, aka PULLED DOWN.
  DebouncedResults = j;
  if (INDEX >= 10) INDEX = 0;
  //INDEX loop around;
  //  Serial.println("triggered");
}

void lcdInit() {
  //RST PROCESS 1#: PULL RST DOWN WITHIN 100MS OF VCC UP. THEN PULL RST UP.
  pinMode(VCC, OUTPUT);
  digitalWrite(VCC, LOW); //INMIDIATELY POWER 5110 OFF
  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH); //SET UP THE RST FIRST
  digitalWrite(VCC, HIGH); //POWER ON
  delay(1);
  digitalWrite(RST, LOW); //RST WITHIN 100MS OF POWER ON
  delay(1);
  digitalWrite(RST, HIGH); //RST FOR AT LEAST 1MS, RST COMPLETE
  //INIT PROCESS: SETUP ALL OTHER PINS
  pinMode(DIN, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(CE, OUTPUT);
  pinMode(DC, OUTPUT);
  digitalWrite(CE, HIGH); //PULL CE HIGH TO AVOID ERRORS, CE IS ACTIVE LOW
  //RST PROCESS 2#: DEFINE ALL RAM TO 0, WRITE TO DISPLAY RAM FIRST THEN POWER THE LCDS ON
  cWrite((uint8_t)0x21); // POWER DOWN MODE OFF, EXTENDED INSTRUCTION MODE ON
  digitalWrite(CLK, LOW); //PREPARE TO POWER-WASH THE RAM TO 0
  digitalWrite(CE, LOW);
  for (int i = 0; i < 587; i++) {
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);
  }//504 BLOCKS OF uint8_tS OF DATA WRITTEN. 84*48/8=504
  digitalWrite(CE, HIGH);
  //RST PROCESS 3#： SET UP ALL THE PARAMETERS AND TURN THE LCD DRIVE ON
  cWrite((uint8_t)0x90); //SETUP VOP ACCORDING TO DATASHEET EXAMPLE, BUT FOR SOME REASON THE BIAS PARAMETERS ARE NOT SET IN THE EXAMPLE, I WILL LEAVE THEM BE AT THE MOMENT
  cWrite((uint8_t)0x20); //SET PD = 0 V = 0 AND H = 0
  cWrite((uint8_t)0xC); //SET D =1 AND E = 0 TURNING ON THE DISPLAY
  //RST PROCESS COMPLETE
}

void cWrite (uint8_t com) {
  //function just for write commands to the lcd controller
  digitalWrite(DC, LOW); //COMMAND DC ACTIVE LOW, PREPARE DC BEFORE SHIFOUT
  digitalWrite(CLK, LOW); //WHEN DATA IS SAMPLED ON THE RISING EDGE OF CLK, BEFORE SHIFTOUT CLK NEED TO BE LOW
  digitalWrite(CE, LOW);//CE ACTIVE LOW
  delay(1); // LET CE SINK IN A BIT
  shiftOut(DIN, CLK, MSBFIRST, com);
  digitalWrite(CE, HIGH);//CE ACTIVE LOW
  digitalWrite(DC, HIGH);  //KEEP DC HIGH IN CASE OF ERRORS
}

void dCard (uint8_t col, uint8_t row) {
  // draw the left and right edges of a card
  cWrite((uint8_t)(0x40 + row - 1)); //SET Y, OR ROW. 0X40 IS 1000000
  cWrite((uint8_t)(0x80 + (col - 1) * 12 + 3)); //SET X, OFFSET BY +3 FOR DOT POSITION
  digitalWrite(CE, LOW);
  digitalWrite(CLK, LOW);
  shiftOut(DIN, CLK, MSBFIRST, 0XFF);
  digitalWrite(CE, HIGH);
  cWrite((uint8_t)(0x80 + (col - 1) * 12 + 8)); //OFFSET BY +3 THEN +5 FOR ANOTHER SET OF DOTS
  digitalWrite(CE, LOW);
  digitalWrite(CLK, LOW);
  shiftOut(DIN, CLK, MSBFIRST, 0XFF);
  digitalWrite(CE, HIGH);
}

void dNum (uint8_t col, uint8_t row, uint8_t num) {
  // draw the middle part of the card
  cWrite((uint8_t)(0x40 + row - 1)); //SET Y, OR ROW. 0X40 IS 1000000
  cWrite((uint8_t)(0x80 + (col - 1) * 12 + 4)); //SET X, OFFSET BY +4 FOR NUMBER POSITION
  digitalWrite(CE, LOW);
  digitalWrite(CLK, LOW);
  for (uint8_t i = 0; i < 4; i++) {
    shiftOut(DIN, CLK, MSBFIRST, nPart[(num - 1)][i]);
  }
  digitalWrite(CE, HIGH);
}

void deckShuffle() {
  uint8_t deckSize = sizeof(DECK);
  while (deckSize > 1) {
    uint8_t temp;
    uint8_t ran;
    randomSeed(analogRead(0)); // this is important to generate a new deck every time this function is called.
    ran = (uint8_t)(random(deckSize - 1));
    temp = DECK[(deckSize - 1)];
    DECK[(deckSize - 1)] = DECK[ran];
    DECK[ran] = temp;
    deckSize--;
  }
  //  deckSize = 52;
}

void PressedButton() {
  uint8_t DebouncedR = (DebouncedResults >> 2); //the PINB will return pin 6 to 13. and last bit of this PINB is pin 8 which is not needed. pin 10 to 13 are used here.
  //acroding to https://www.arduino.cc/en/Hacking/PinMapping168 pin 10 to pin 13 are mapped to PB2 to PB5, so shifted 2 times will get pin 10 PB2 to be the left most bit.
  uint8_t i;
  // this bits of a for code block, check 4 bits of the debouncedresults, one by one
  // first it checks if the button is now being pressed, due to active low, when 0 means being pressed now
  // if being pressed, then check if this button has been pressed before, if no, PastPressed = 0, then record the press, and change the PastPress for this bit into 1;
  // if has been pressed before then this press does not count, ButtonState will be changed into 0, and Pastpressed stays 1
  // if the button now is not being pressed, then PastPressed and ButtonState will be 0;
  for (i = 0; i < 4; i ++) {
    if (bitRead(DebouncedR, i) == 0) {
      // button is being pressed
      if (bitRead(PastPressed, i) == 0) {
        // button has not be pressed in the past
        bitSet(ButtonState, i); // now the program will know button is pressed.
        bitSet(PastPressed, i); // and the pressed history is recorded
      } else {
        // button is pressed before
        bitClear(ButtonState, i); // if the button is pressed before, and now it is still being pressed, then this press does not count
      }
    } else {
      // button is not being pressed
      bitClear(PastPressed, i);
      bitClear(ButtonState, i);
    }
  }
}

void DeckDisplay() {
  // a function that runs in the setup() process, and displays the shuffled deck, 7 cols and 5 rows
  // directly from the shuffled deck array.
  // this will only run once, part of the initialization process.
  uint8_t colNum, rowNum;
  for (colNum = 1; colNum < 8; colNum++) {
    for (rowNum = 1; rowNum < 6; rowNum++) {
      dCard(colNum, rowNum);
      dNum(colNum, rowNum, DECK[(rowNum + (colNum - 1) * 5) - 1]); //DECK will be [col1row1,col1row2,col1row3....col1row5,col2row1...] and DECK is an array it starts from 0, so -1
    }
  }
  // then the revealed card at the beginning of the game is displayed.
  dCard(6, 6);
  dNum(6, 6, DECK[35]); // next card after drawing 5*7 cards, and DECK start from 0
}

void NumDraw(uint8_t sX, uint8_t sY, uint8_t ModeSelect, int DrawNum )
{
  // used for draw numbers for scores and remaining card numbers.
  // sX and sY are starting positions, X is 0-83, y is 0-5, for precise position control
  // ModeSelect is to select from 2 digit mode 0 and 4 digit mode 1. No. of remaining card only needs 2, and score needs 4.
  // DrawNum is the number to be drawn.
  cWrite((uint8_t)(0x40 + sY));
  cWrite((uint8_t)(0x80 + sX)); // set start position of X and Y
  digitalWrite(CLK, LOW); // start the erase process.
  digitalWrite(CE, LOW);
  for (uint8_t i = 0; i < ((ModeSelect + 1) * 2); i++) {
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0); // according to the mode, 2 digit or 4, if the mode is 0, then the code is run twice, to erase 2 digits, 3x each
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);// each digit is 3 pixels wide.
  }
  digitalWrite(CE, HIGH); // end of the erase process
  int tempNum; // check if the input DrawNum is bigger than it should be, if so a 99 or 9999 will be displayed.
  if (ModeSelect == 0) {
    if (DrawNum > 99) {
      tempNum = 99;
    } else {
      tempNum = DrawNum;
    }
  } else {
    if (DrawNum > 9999) {
      tempNum = 9999;
    } else {
      tempNum = DrawNum;
    }
  }
  cWrite((uint8_t)(0x40 + sY));
  cWrite((uint8_t)(0x80 + sX)); // move back to the starting position and ready to draw number
  digitalWrite(CLK, LOW); // start the number writing process
  digitalWrite(CE, LOW);
  if (ModeSelect == 0) {
    // 2 digit mode
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)(NumForDisplay[((tempNum - (tempNum % 10)) / 10)][0]));
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)(NumForDisplay[((tempNum - (tempNum % 10)) / 10)][1]));
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)(NumForDisplay[(tempNum % 10)][0]));
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)(NumForDisplay[(tempNum % 10)][1]));
  } else {
    // 4 digit mode, ModeSelect = 1
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)(NumForDisplay[((tempNum - (tempNum % 1000)) / 1000)][0]));
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)(NumForDisplay[((tempNum - (tempNum % 1000)) / 1000)][1]));
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)(NumForDisplay[((tempNum % 1000 - tempNum % 100) / 100)][0]));
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)(NumForDisplay[((tempNum % 1000 - tempNum % 100) / 100)][1]));
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)(NumForDisplay[((tempNum % 100 - tempNum % 10) / 10)][0]));
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)(NumForDisplay[((tempNum % 100 - tempNum % 10) / 10)][1]));
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)(NumForDisplay[(tempNum % 10)][0]));
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)(NumForDisplay[(tempNum % 10)][1]));
  }// this part of the code is messy indeed, but it's really hard to work out a concise block of code, so a dirty way it is.
  digitalWrite(CE, HIGH);
}

void ArrowDraw(uint8_t col, uint8_t row) {
  // used to erase last arrow and draw a new one into col, row. always on the right hand of a card.
  // move to last arrow position for erase
  // use global arrow last location
  cWrite((uint8_t)(0x40 + ArrowLastPosition[1] - 1));
  cWrite((uint8_t)(0x80 + (ArrowLastPosition[0] - 1) * 12 + 9)); // +9 keep the arrow on the right hand side of the card
  // erase
  digitalWrite(CLK, LOW);
  digitalWrite(CE, LOW);
  for (uint8_t i = 0; i < 3; i++) {
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0); // write 3 consective 0 into the display RAM
  }
  digitalWrite(CE, HIGH);
  // move to new position
  cWrite((uint8_t)(0x40 + row - 1));
  cWrite((uint8_t)((0x80 + (col - 1) * 12 + 9))); // moved to col, row
  // write a new arrow
  digitalWrite(CLK, LOW);
  digitalWrite(CE, LOW);
  for (uint8_t i = 0; i < 3; i++) {
    shiftOut(DIN, CLK, MSBFIRST, ArrowDisplay[i]); // write 3 lines into RAM, forming an arrow
  }
  digitalWrite(CE, HIGH);
  // last part, write current col and row into ArrowLastPosition
  ArrowLastPosition[0] = col;
  ArrowLastPosition[1] = row;
}

void CardErase(uint8_t col, uint8_t row) {
  // a function to erase card at location on col row
  // this function can be used when a set action is taken, then the set card will be erased as well as the revealed card
  // a new card then is needed to be drawn at the revealed card location
  // for a reveal action, only the revealed card location needed to be erased
  // as for drawing a new card, old dCard and dNum can be used.
  cWrite((uint8_t)(0x40 + row - 1)); //SET Y, OR ROW. 0X40 IS 1000000
  cWrite((uint8_t)(0x80 + (col - 1) * 12 + 3)); //SET X, OFFSET BY +3 FOR DOT POSITION, the same as dCard, starting at the edge of the card, left
  digitalWrite(CLK, LOW);
  digitalWrite(CE, LOW);
  for ( uint8_t i = 0; i < 6; i++) {
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);  // write 6 0s into the display ram. 6 lines in the x direction is enough to erase a card including edges.
  }
  digitalWrite(CE, HIGH);
}

void ScoreKeeping() {
  int AddedScore = 0;
  unsigned long ttime = millis() - TempTime;
  // CALCULATE ADD SCORE BASED ON TIMER AND CP
  if (ttime > 10000) AddedScore = CP;
  if (ttime > 5000 && ttime <= 10000) AddedScore = 2 * CP;
  if (ttime > 1000 && ttime <= 5000) AddedScore = 5 * CP;
  if (ttime <= 1000) AddedScore = 10 * CP;
  // CALCULATE TOTAL SCORE
  Score = Score + AddedScore;
  // UPDATE SCORE DISPLAY
  NumDraw(3, 5, 1, Score);
}

void EndGame() {
  // 1 ERASE SCREEN
  digitalWrite(CLK, LOW); //PREPARE TO POWER-WASH THE RAM TO 0
  digitalWrite(CE, LOW);
  for (int i = 0; i < 587; i++) {
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);
  }//504 BLOCKS OF uint8_tS OF DATA WRITTEN. 84*48/8=504
  digitalWrite(CE, HIGH);
  // 2 calculate the final score
  Score = Score + RemainingCards * 10;
  // 3 end screen 1
  cWrite((uint8_t)(0x40 + 0)); // LINE 1
  cWrite((uint8_t)(0x80 + 5)); // LINE 1 X POSITION
  WordPrint(ED1, sizeof(ED1));
  cWrite((uint8_t)(0x40 + 1)); // LINE 1
  cWrite((uint8_t)(0x80 + 2)); // LINE 1 X POSITION
  WordPrint(ED2, sizeof(ED2));
  NumDraw(35, 3, 1, Score);
  delay(3000);
  digitalWrite(CLK, LOW); //PREPARE TO POWER-WASH THE RAM TO 0
  digitalWrite(CE, LOW);
  for (int i = 0; i < 587; i++) {
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);
  }//504 BLOCKS OF uint8_tS OF DATA WRITTEN. 84*48/8=504
  digitalWrite(CE, HIGH);
  // 4 end screen 2
  // read the high scores from eeprom
  int a = EEPROMReadInt(1);
  int b = EEPROMReadInt(2);
  int c = EEPROMReadInt(3);
  uint8_t AP = 0; // arrow position
  if (Score > c) {
    c = Score;
    AP = 3; // 3rd position
  }
  if (Score > b) {
    c = b;
    b = Score;
    AP = 2;
  }
  if (Score > a) {
    b = a;
    a = Score;
    AP = 1;
  }
  EEPROMWriteInt(1, a);
  EEPROMWriteInt(2, b);
  EEPROMWriteInt(3, c);
  cWrite((uint8_t)(0x40 + 1)); // LINE 2
  cWrite((uint8_t)(0x80 + 5)); // LINE 2 X POSITION
  WordPrint(ED3, sizeof(ED3));
  NumDraw(30, 2, 0, 1);
  NumDraw(40, 2, 1, a); // high score 1
  NumDraw(30, 3, 0, 2);
  NumDraw(40, 3, 1, b); // high score 2
  NumDraw(30, 4, 0, 3);
  NumDraw(40, 4, 1, c); // high score 2
  if (AP > 0 ) ArrowDraw(5, (AP + 2));
  delay(5000);
  digitalWrite(RESET, LOW); // HARDWARE + SOFTWARE RESET
  while (1) {}

}

void StartGame() {
  // 1 ERASE SCREEN
  digitalWrite(CLK, LOW); //PREPARE TO POWER-WASH THE RAM TO 0
  digitalWrite(CE, LOW);
  for (int i = 0; i < 587; i++) {
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);
  }//504 BLOCKS OF uint8_tS OF DATA WRITTEN. 84*48/8=504
  digitalWrite(CE, HIGH);
  // 2 WRITE 3 LINES
  cWrite((uint8_t)(0x40 + 0)); // LINE 1
  cWrite((uint8_t)(0x80 + 25)); // LINE 1 X POSITION
  WordPrint(SS1, sizeof(SS1));
  delay(500);
  cWrite((uint8_t)(0x40 + 2));
  cWrite((uint8_t)(0x80 + 5)); //line 3 the 6 x
  WordPrint(SS2, sizeof(SS2));
  delay(500);
  cWrite((uint8_t)(0x40 + 5));
  cWrite((uint8_t)(0x80 + 9)); //line 6 the 10 x
  WordPrint(SS3, sizeof(SS3));
  delay(1000);
  // 3 WIPE RAM AGAIN
  digitalWrite(CLK, LOW); //PREPARE TO POWER-WASH THE RAM TO 0
  digitalWrite(CE, LOW);
  for (int i = 0; i < 587; i++) {
    shiftOut(DIN, CLK, MSBFIRST, (uint8_t)0x0);
  }//504 BLOCKS OF uint8_tS OF DATA WRITTEN. 84*48/8=504
  digitalWrite(CE, HIGH);
}



void WordPrint(uint8_t index[], uint8_t l) {
  // write a line of words, from index array pointing to the alphadisplay array
  digitalWrite(CE, LOW);
  digitalWrite(CLK, LOW);
  for (uint8_t i = 0; i < l; i++) {
    for (uint8_t j = 0; j < 7; j++) {
      shiftOut(DIN, CLK, MSBFIRST, AlphaDisplay[index[i]][j]); // first print out one letter from the alphadisplay
    }
    shiftOut(DIN, CLK, MSBFIRST, B00000000); // add one col of 0, then print out the whole index of letters and space
  }
  digitalWrite(CE, HIGH);
}

void EEPROMWriteInt(int p, int value)
{ // p is for position, each position holds 2 addresses
  byte two = (value & 0xFF);
  byte one = ((value >> 8) & 0xFF);

  EEPROM.update((p - 1) * 2, two);
  EEPROM.update((p - 1) * 2 + 1, one);
}

int EEPROMReadInt(int p)
{ // p is for position, each position holds 2 addresses
  long two = EEPROM.read((p - 1) * 2);
  long one = EEPROM.read((p - 1) * 2 + 1);

  return ((two << 0) & 0xFFFFFF) + ((one << 8) & 0xFFFFFFFF);
}

void setup() {
  digitalWrite(RESET, HIGH);
  pinMode(RESET, OUTPUT);   // PULL RESET PIN HIGH IN ORDER TO DO A SOFTWARE RESET LATER
  cli();//imidiately disable all interrupts during reset and other processes. interrupts are used here only for button. if no button are needed ,cli()
  lcdInit();
  deckShuffle();
  sei();
  StartGame();
  cli();
  DeckDisplay();
  NumDraw(75, 5, 0, RemainingCards); // the remaining card number is 16, with 35 on the deck, 1 already revealed, and 52 cards in total. the no. of remaining cards are in position (75,5), and it's 2 digit mode
  NumDraw(3, 5, 1, Score); // score display. start with 0. start position is (3,5)
  ArrowDraw(ArrowLastPosition[0], ArrowLastPosition[1]);
  //bits about timer2 interrupts, more on this :https://www.teachmemicro.com/arduino-timer-interrupt-tutorial/
  TIMSK2 = (TIMSK2 & B11111110) | 0x01; //timer 2 overflow register byte
  TCCR2B = (TCCR2B & B11111000) | 0x06; //timer 2 will trigger an overflow for every 4ms, this might be too harsh for the processor but we will have to see. use 0x07 for 16ms trigger.
  // but timer overflow might have something to do with clock speed of the processor, in this case a 8mhz arudino might have different trigger time
  pinMode(10, INPUT_PULLUP);
  pinMode(11, INPUT_PULLUP);
  pinMode(12, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);
  //  Serial.begin(9600);
  sei();
  delay(1000); // this delay here is used to make sure debouncing interrupts will not throw out random stuff.
  TempTime = millis();
}

void loop() {

  PressedButton();
  if (bitRead(ButtonState, 0) == 1) {
    //    Serial.println("pin10 pressed!");
    // if LEFT button is pressed
    for (uint8_t i = ArrowLastPosition[0]; i > 1; i--) {
      // count down to col 1
      if (CardStatues[(i - 2)] > 0) {
        ArrowDraw((i - 1), CardStatues[(i - 2)]);
        break;// arrow moved and new set of arrow location is recorded
      }
    }
  }
  if (bitRead(ButtonState, 1) == 1) {
    //    Serial.println("pin11 pressed!");
    for (uint8_t i = ArrowLastPosition[0]; i < 7; i++) {
      // count down to col 1
      if (CardStatues[i] > 0) {
        ArrowDraw((i + 1), CardStatues[i]);
        break;// arrow moved and new set of arrow location is recorded
      }
    }
  }
  if (bitRead(ButtonState, 2) == 1) {
    //    Serial.println("pin12 pressed!");
    if ((max(DECK[((ArrowLastPosition[0] - 1) * 5 + ArrowLastPosition[1] - 1)], DECK[LastRevealed]) - min(DECK[((ArrowLastPosition[0] - 1) * 5 + ArrowLastPosition[1] - 1)], DECK[LastRevealed])) == 1 || (max(DECK[((ArrowLastPosition[0] - 1) * 5 + ArrowLastPosition[1] - 1)], DECK[LastRevealed]) - min(DECK[((ArrowLastPosition[0] - 1) * 5 + ArrowLastPosition[1] - 1)], DECK[LastRevealed])) == 12) { //first || and later if removed, it wont ramp from k to 1
      // FOR SOCRES
      CP++;
      if (CP > 5) CP = 5;
      ScoreKeeping(); // calculate socre and update score display
      TempTime = millis();
      // compare current card with the revealed card, according to the DECK array,then
      // 1 erase both cards
      CardErase(ArrowLastPosition[0], ArrowLastPosition[1]);
      CardErase(6, 6);
      // 2 draw cerrent card to the revealed card position
      dCard(6, 6);
      dNum(6, 6, (DECK[((ArrowLastPosition[0] - 1) * 5 + ArrowLastPosition[1] - 1)]));
      // 3 update revealed card index
      LastRevealed = (ArrowLastPosition[0] - 1) * 5 + ArrowLastPosition[1] - 1;
      // 4 update CardStatues[], update new card/arrow position, and if this is the last card then game over screen
      CardStatues[(ArrowLastPosition[0] - 1)] = CardStatues[(ArrowLastPosition[0] - 1)] - 1; // update cardstatues[]
      if (CardStatues[(ArrowLastPosition[0] - 1)] > 0) {
        // not last card in this col, after updating the cardstatues{}
        ArrowDraw(ArrowLastPosition[0], (ArrowLastPosition[1] - 1)); //draw arrow upwards and update the current card/arrow position
      } else {
        // after set if it was thelast card in the col, then
        for (uint8_t i = ArrowLastPosition[0]; i > 1; i--) {
          // count down to col 1, do a left search
          if (CardStatues[i - 2] > 0) {
            ArrowDraw((i - 1), CardStatues[i - 2]);
            Seeked = 1; // mark the fact that the arrow has found a card to move
            break;// arrow moved and new set of arrow location is recorded
          }
        }
        if (Seeked == 0 ) {
          // left search is not successful, do a right search
          for (uint8_t i = ArrowLastPosition[0]; i < 7; i++) {
            // count down to col 1
            if (CardStatues[i] > 0) {
              ArrowDraw((i + 1), CardStatues[i]);
              Seeked = 1; // mark the fact that the right move is done.
              break;// arrow moved and new set of arrow location is recorded
            }
          }
        }
        if (Seeked == 0) {
          // if, arrow can not find a place to be upwards left and right, then it is the last card on the deck, game over
          // *** game over code here ***
          EndGame();
        }
        Seeked = 0; // turn Seeked flag back to 0
      }
    }
  }
  if (bitRead(ButtonState, 3) == 1) {
    // for scores. refresh cp, and update timer
    CP = 0;
    TempTime = millis();
    if (RemainingCards == 0) {
      EndGame();
    }
    RemainingCards = RemainingCards - 1;
    NumDraw(75, 5, 0, RemainingCards); // update the no. of RemainingCards
    CardErase(6, 6); // erase the revealed card
    dCard(6, 6);
    dNum(6, 6, DECK[(51 - RemainingCards)]); // this is the card index to be revealed
    LastRevealed = 51 - RemainingCards; // LastRevealed will be changed when set action is taken, it will = the arrow position card. so when a reveal action is taken, this will be changed to the last revealed card index
  }
}
