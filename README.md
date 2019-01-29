# arduino-golf-solitaire
a simple golf solitaire game (with "wrapping", Ace on a King, or King on an Ace), written for nokia 5110 display and a 3.3v pro mini controller. Inspired by pocket card jockey by game freak.

(it is my 2nd major arduino project and my first github repo, the codes are bond to be a bit messy so pardon me)

my working prototype:

![the prototype](https://github.com/echodoomyou/arduino-golf-solitaire/blob/master/prototype/prototype.jpg)

version 0.1 

1/27/19

## features
* working reasonably well enough
* fast
* a score system rewarding both skill and reaction
* high score storage in EEPROM
* rather small size after compilation
* soft reset after game over

## wiring

### 1. from pro mini to Nokia 5110 display

pro mini--->5110

pin 9--->CLK

pin 8--->CE

pin 7--->DIN

pin 6--->DC

pin 5--->RST

pin 4--->VCC

GND --->GND

VCC--->BL(optional, for back light only)

### 2. other connections

pro mini--->OTHER

pin A0--->pro mini RST

pin 10-13--->4 push buttons then to GND

## game rules

https://en.wikipedia.org/wiki/Golf_(patience) but with "wrapping" (Ace on a King, or King on an Ace).

## scores

see ScoreKeeping() and EndGame(). basically:

* faster plays get more scores
* consective plays get more scores
* bonus points for unrevealed cards when the deck(table) is cleared

(for the moment my best score is 743.)


## notes and limitations

* display part of the code is written from ground up to save some space, instead of using lib like u8g2
* software SPI is used, due to the fact it is a card game, software SPI speed is not of top priority
* interrupt based debouncing is used, with alright performance, but not perfect, code is based on http://www.ganssle.com/debouncing-pt2.htm
* part of the Amstrad CPC extended font is recreated, from https://fontstruct.com/fontstructions/show/25590/amstrad_cpc_extended
* part of the code of EEPROM reading and writing int type data is from https://www.instructables.com/id/two-ways-to-reset-arduino-in-software/
* number might be a bit small but to accomadate the small display, a 2 pixel width number font is created, might need to get used to
* Sketch uses 5420 bytes (17%) of program storage space. Maximum is 30720 bytes. Global variables use 409 bytes (19%) of dynamic memory, leaving 1639 bytes for local variables. Maximum is 2048 bytes.

## future updates
* rewrite parts of the code to utilize hardware SPI
* try other types of software debouncing, current performance can be improved
