#include <Arduino.h>

/*
 * Title: Reflex Game
 *
 * Description: An Arduino based reflex game. Uses 4 lights/buttons
 * combinations. When the light is on user pushes the corresponding button.
 * The light turns off and another turns on in random. The game counts how
 * many the user does correctly in a predifined time (30 secs).
 * Arduino uses a 4 zone 8x8 LED matrices display driven by MAX7219 to display
 * the score on the top 2 and the time on the bottom 2. 
 * We are using also a coin acceptor CH92xx. The game starts when a coin is 
 * inserted.
 *
 * Author: Jann Kruse, Nikos Kanelakis, Dimitris Koukoulakis
 * Licence: General Public Licence (GPL) v3
 * Company: CommonsLab
 * Site: http://commonslab.gr
 * Date: 01/09/2016
 */

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#if USE_COIN_ACCEPTOR
#include "notes.h"
#endif
#if USE_LIBRARY_SPI
#include <SPI.h>
#endif

//Use of LED matrix display
#define USE_LED_SCREEN 1

//Use of Coin Acceptor
#define USE_COIN_ACCEPTOR 1

//GAME VARIABLES
#define GAME_TIME 30
//DEBUG MORE
#define DEBUG 0 //if set to 1 will print messages on Serial port.

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define MAX_DEVICES 4
#define MAX_ZONES   4
#define ZONE_SIZE (MAX_DEVICES/MAX_ZONES)   // integer multiple works best

//Connection PIN for Coin Acceptor
#define PULSE_INPUT 12
//Connetion PINS for LED matrix
#define CLK_PIN   13
#define DATA_PIN  11
#define CS_PIN    10

// Hardware SPI connection
MD_Parola P = MD_Parola(CS_PIN, MAX_DEVICES);
// Arbitrary output pins
// MD_Parola P = MD_Parola(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Global variables
int btn_pins[] = {3,5,7,9}; //Button PINs
int led_pins[] = {2,4,6,8}; //LED PINs
const int MAX_CONNS = 4;
int buzz_minus = A0; 
int buzz_plus = A3;
int countdown = GAME_TIME;
int score = 0; //Loop adds one point first time it runs
bool game_on = false;

#if USE_LED_SCREEN
//Digit variables for LED matrix
int d1 = 0;
int d2 = 0;
int d3 = 0;
int d4 = 0;
int brightness = 10;
//Used for countdown
unsigned long currentMillis = 0;
const long interval = 1000;
unsigned long previousMillis = 0;
#endif

int idx = -1;

#if USE_COIN_ACCEPTOR
const char* CURRENCY = "EUR";   // Currency unit of the coin acceptor
const int   MAX_PULSES = 99;    // Not more that so many pulses make sense. Stop counting here and complain.
const int   centPerPulse = 50;  // This many 1/100th of a currency unit are counted per pulse.

// Define here some timing constants according to the settings of the coin acceptor.
enum timing
  {
    T_MIN = 25,     // The PULSE_INPUT shourd be LOW or HIGH for at least this long, otherwise it's noise.
    T_MAX = 150,    // The PULSE_INPUT shourd be LOW or HIGH for at most this long, otherwise it's not a pulse.
  };

// States of the finite state machine:
enum state
  {
  // The first (negative) states are initial, before the pulse sequence starts.
    INIT          = -1, // state after power-up, reset, or after reporting a pulse sequence.
    START         = -2, // Start here wating for a pulse.
    START_CONFIRM = -3, // Input just went HIGH for the first time.

  // The next (positive) states are to count the pulses in the sequence.
    LO            =  0, // Counting pulses, input pin is LOW  (0 V).
    HI            =  1, // Counting pulses, input pin is HIGH (5 V).
    LO_CONFIRM    =  2, // Counting pulses, input pin just went LOW.
    HI_CONFIRM    =  3, // Counting pulses, input pin just went HIGH.
  };

// Enable or disable debugging features here.
bool USE_STATUS_LED = false;
bool VERBOSE = true;

//SoftwareSerial Serial(SERIAL_RX, SERIAL_TX); // Use software serial for compatibility with ATtiny85.

// Variables
unsigned char pulses; // Holds the number of pulses that have been counted lately.
unsigned long tLow;  // Holds the millis() when pin went LOW.
unsigned long tHigh;    // Holds the millis() when pin went HIGH.
state currentState;   // Holds the current state of the finite state machine.
#endif //USE_COIN_ACCEPTOR

void setup(void) {

  Serial.begin(9600);

#if USE_LED_SCREEN
  InitLEDScreen();
#endif

  for(int i=0; i<MAX_CONNS; i++) {
    pinMode(led_pins[i], OUTPUT);
    pinMode(btn_pins[i], INPUT_PULLUP);
  }

  // Set up the pins for the buzzer:
  pinMode(buzz_plus, OUTPUT);
  pinMode(buzz_minus, OUTPUT);
  digitalWrite(buzz_minus, LOW);    // pull the minus pin to 0 Volt by making it "LOW"

#if USE_COIN_ACCEPTOR
  currentState = INIT;
  pinMode(PULSE_INPUT,  INPUT);
  digitalWrite(PULSE_INPUT, LOW); // Dectivate internal pull-up resistor. (So it's not HIGH when idle.)  
#endif
  Serial.println("Starting...");
  gameStart();
}

void generateRandom()
{
  randomSeed(analogRead(A5)*analogRead(A6));
  for(int i=0; i<10; i++) {
    int rnd = int(random(MAX_CONNS));
    if(rnd != idx) {
      idx = rnd;
      break;
    }
  }
}

void checkTime()
{
  if(currentMillis - previousMillis > interval) {
    Serial.print("Time:");
    Serial.println(countdown);
    previousMillis = currentMillis;
    #if USE_LED_SCREEN
      PrintTime();
    #endif //USE_LED_SCREEN
  }
}

void loop(void)
{
  while(game_on) {
    currentMillis = millis();

    //Turn OFF all LEDs
    clearLeds();
    //Turn ON the random LED
    digitalWrite(led_pins[idx], HIGH);

    //Read all the buttons
    for (int i=0; i < MAX_CONNS; i++) {
      delay(10);
      if( (digitalRead(btn_pins[i]) == HIGH) && (i == idx)) {
        clearLeds();  
        AddPoint();
        while ( (digitalRead(btn_pins[i]) == HIGH) && (i == idx)) {}; // wait until butten is released
        generateRandom(); // select another random led
        return;
      } else if (digitalRead(btn_pins[i]) == HIGH) {
        Error();
      }
    }

    checkTime();

    if (countdown < 0) {
      gameEnd();
      return;
    }
  }//while game_on

#if USE_COIN_ACCEPTOR
  switch (currentState) {
    case INIT:
    // ENTRY:
      pulses = 0;

    // WAIT FOR EVENT:
      while (not (digitalRead(PULSE_INPUT) == HIGH)) {}; // Wait for pin to go HIGH.
      // EVENT HAPPENED! PIN WENT HIGH.

    //EXIT:
      tHigh = millis();
      currentState = START;
    break;


    case START:
    //ENTRY:
      if (VERBOSE) Serial.println("S");

    // WAIT FOR EVENT:
      while (digitalRead(PULSE_INPUT) == HIGH) {
        #if USE_LED_SCREEN
          //printMsg();
        #endif //USE_LED_SCREEN
        }; // Wait until pin goes LOW. (I.e. wait while pin stays HIGH.)
      // EVENT HAPPENED! PIN WENT LOW.

    //EXIT:
      currentState = START_CONFIRM;
    break;

  
    case START_CONFIRM:
    //ENTRY:
      tLow = millis(); // Save system time when pin went LOW.
      if (VERBOSE) Serial.println("SC");

    // WAIT FOR EVENT:
      while (   (digitalRead(PULSE_INPUT) == LOW) // Wait until pin goes HIGH. (Wait while pin stays LOW.)
             && (millis() - tLow <= T_MIN)) {}; // as long as time reaches minimum (confirmed LOW).
      // EVENT HAPPENED! Either the pin went HIGH again quickly (noise?) or it's a stable LOW, i.e. a pulse.

    //EXIT:
      if (millis() - tLow >= T_MIN) { // Good pulse...probably.
        currentState = LO;
      } else {
        currentState = START;             // Bad pulse... Back to START.
      }
    break;

    //---------------------------------------------
    case LO:
    //ENTRY:
      if (VERBOSE) Serial.println("L");

    // WAIT FOR EVENT:
      while (   (digitalRead(PULSE_INPUT) == LOW)    // Wait while pin is stays LOW
             && (millis() - tLow <= T_MAX)) {}; // as long as time does not exceed maximum.
      // EVENT HAPPENED! Either the pin went HIGH again (end of pulse or noise) or something's wrong...

    //EXIT:
      if (millis() - tLow >= T_MAX) { // This is wrong...
        currentState = INIT;
      } else {
        currentState = HI_CONFIRM;
      }
    break;

    //---------------------------------------------
    case HI_CONFIRM:
    //ENTRY:
      tHigh = millis(); // Save system time when pin went HIGH.
      if (VERBOSE) {Serial.print("HC -");Serial.println(millis() - tLow);}

    // WAIT FOR EVENT:
      while (   (digitalRead(PULSE_INPUT) == HIGH)   // Wait until pin goes LOW (while pin is stays HIGH)
             && (millis() - tHigh <= T_MIN)) {};  // as long as time reaches minimumim (confirmed HIGH).
      // EVENT HAPPENED! Either the pin went LOW again (noise?) or it's a stable HIGH, i.e. end of pulse.

    //EXIT:
      if (millis() - tHigh >= T_MIN) { // Good pulse... ... Count this!
        pulses = pulses +1;
        if (pulses > MAX_PULSES) { currentState = INIT;} // Too many pulses !! ??
        if (VERBOSE) {Serial.print("Ps:");Serial.println(pulses);}
        currentState = HI;
      } else {                       // Bad pulse... (noise)
        currentState = LO;
      }
    break;

    //---------------------------------------------
    case HI:
    //ENTRY:
      if (VERBOSE) Serial.println("H");

    // WAIT FOR EVENT:
      while (   (digitalRead(PULSE_INPUT) == HIGH)  // Wait while pin is stays HIGH (until it goes low)
             && (millis() - tHigh <= T_MAX)) {}; // as long as time does not exceed maximum.
      // EVENT HAPPENED! Either the pin went LOW again or something's wrong...

    //EXIT:
      if (millis() - tHigh >= T_MAX) { // If the pin stays high, that's the end of the pulse sequence.
        //COIN INSERTED
        reportPulses(pulses);
        gameStart();
        currentState = INIT;
      } else {
        currentState = LO_CONFIRM;         // Otherwise there's (probably) another pulse (to be confirmed).
      }
    break;

    //---------------------------------------------
    case LO_CONFIRM:
    //ENTRY:
      tLow = millis(); // Save system time when pin went HIGH.
      if (VERBOSE) {Serial.print("LC -");Serial.println(millis() - tHigh);}

    // WAIT FOR EVENT:
      while (   (digitalRead(PULSE_INPUT) == LOW)     // Wait while pin is stays LOW
             && (millis() - tLow <= T_MIN)) {}; // as long as time reaches minimumim (confirmed LOW).
      // EVENT HAPPENED! Either the pin went HIGH again very quickly (noise?) or it's a stable LOW, i.e. a pulse.

    //EXIT:
      if (millis() - tHigh >= T_MIN) { // Stable low.
        currentState = LO;
      } else {                            // Bad pulse... Back to HI.
        currentState = HI;
      }
    break;

    default:
     if (VERBOSE) Serial.println("FELL INTO DEFAULT CASE!!!");
    break;
  }  
#endif  
}

#if USE_LED_SCREEN
//Prints 2 digit score to the LED matrix
void PrintScore() {
  char c1[2];
  char c2[2];
  //Reset score if reaches 100
  if (score > 99) {
    score = 0;
  }
  //Get last digit
  d1 = score % 10;
  //If it has a second digit, get the first one
  if (score >=10) {
    d2 = score / 10 % 10;
  } else {
    //Set first digit to 0
    d2 = 0;
  }
  //Convert each digit to char array
  sprintf(c1,"%01d", d1);
  sprintf(c2,"%01d", d2);
  //Print each digit to the first two LED matrices
  P.displayZoneText(0,c1,CENTER,5, 0,PRINT, NO_EFFECT);
  P.displayZoneText(1,c2,CENTER,5, 0,PRINT, NO_EFFECT);
  P.displayAnimate();
}
#endif //USE_LED_SCREEN

#if USE_LED_SCREEN
//Print countdown of the game time
void PrintTime() {
  char c1[2];
  char c2[2];
  //Get last digit
  d1 = countdown % 10;
  //If it has a second digit, get the first one
  if (countdown >= 10) {
    d2 = countdown / 10 % 10;
  } else {
    //Set first digit to 0
    d2 = 0;
  }
  //Convert each digit to char array
  sprintf(c1,"%01d", d1);
  sprintf(c2,"%01d", d2);
  //Print time
  P.displayZoneText(2,c1,CENTER,5, 0,PRINT, NO_EFFECT);
  P.displayZoneText(3,c2,CENTER,5, 0,PRINT, NO_EFFECT);
  P.displayAnimate();
  //Countdown
  countdown--;
}
#endif //USE_LED_SCREEN
#if USE_LED_SCREEN
void InitLEDScreen() {
  P.begin(MAX_ZONES);
  P.setInvert(false);
  P.setIntensity(brightness);
  for (uint8_t i=0; i<MAX_ZONES; i++)
  {
    P.setZone(i, ZONE_SIZE*i, (ZONE_SIZE*(i+1))-1);
  }
}
#endif //USE_LED_SCREEN
void gameStart() {
  Serial.println("Game starting...");
  game_on = true;
  //Initialize global vars
  score = 0;
  countdown = GAME_TIME;
  //Generate First random number
  generateRandom();
  #if USE_LED_SCREEN
    //InitLEDScreen();
    P.displayZoneText(0,"0",CENTER,5, 0,PRINT, NO_EFFECT);
    P.displayZoneText(1,"0",CENTER,5, 0,PRINT, NO_EFFECT);
    P.displayAnimate();
  #endif

  for(int i=0; i<MAX_CONNS; i++) {
    digitalWrite(led_pins[i], HIGH);
  }
  tone(A3, 440, 200);
  delay(2000);
}

void gameEnd() {
  game_on = false;
#if USE_LED_SCREEN
    //InitLEDScreen();
    P.displayZoneText(2,"*",CENTER,5, 0,PRINT, NO_EFFECT);
    P.displayZoneText(3,"*",CENTER,5, 0,PRINT, NO_EFFECT);
    PrintScore();
#endif  
  Serial.println("Game Finished");
  for(int i=0; i<MAX_CONNS; i++) {
    digitalWrite(led_pins[i], HIGH);
  }
}

void clearLeds() {
  for(int i=0; i<MAX_CONNS; i++) {
    digitalWrite(led_pins[i], LOW);
  }
}

//User pressed wrong button
void Error()
{
  for (int j=0; j<20; j++) {
    for (int i=0; i<10; i++) {
      digitalWrite(buzz_plus, HIGH);
      delayMicroseconds(200);
      digitalWrite(buzz_plus, LOW);
      delayMicroseconds(200);
    }
    delay(10);
    //each loop is 14 millisecs
  }
  previousMillis+=280;
  checkTime();
  //TODO: Use Led matrix on error?
}

//User pressed correct button
void AddPoint()
{
  for (int i=0; i<250; i++)
  {
    digitalWrite(buzz_plus, HIGH);
    delayMicroseconds(200);
    digitalWrite(buzz_plus, LOW);
    delayMicroseconds(200);
  }
  previousMillis+=100;
  checkTime();
  score++;
  Serial.print("Score: ");
  Serial.println(score);
  #if USE_LED_SCREEN
    PrintScore();
  #endif //USE_LED_SCREEN
}

#if USE_COIN_ACCEPTOR
// This routine reports counted pulses as coins through a serial connection.
void reportPulses(unsigned int pulses) {

  if (VERBOSE) {Serial.print("Reporting pulses: ");Serial.println(pulses);}

  Serial.print(CURRENCY);
  if (pulses*centPerPulse < 100)    Serial.print("0");
  else                              Serial.print(pulses*centPerPulse/100); // 100s (full units of the currency)
                                    Serial.print(".");
  if (pulses*centPerPulse%100 < 10) Serial.print("0");
                                    Serial.println(pulses*centPerPulse%100); // (10s and) 1s
};
#endif //USE_COIN_ACCEPTOR
