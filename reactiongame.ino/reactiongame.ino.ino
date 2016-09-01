/*
 * Title: Reflex Game
 *
 * Description: An Arduino based reflex game. Uses 4 lights/buttons 
 * combinations. When the light is on user pushes the corresponding button.
 * The light turns off and another turns on in random. The game counts how
 * many the user does correctly in a predifined time (30 secs).
 * Arduino uses a 4 zone 8x8 LED matrices display driven by MAX7219 to display
 * the score on the top 2 and the time on the bottom 2.
 * 
 * Author: Jann Kruse, Nikos Kanelakis, Dimitris Koukoulakis
 * Licence: General Public Licence (GPL) v3
 * Company: CommonsLab
 * Site: http://commonslab.gr
 * Date: 01/09/2016
 */

#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#if USE_LIBRARY_SPI
#include <SPI.h>
#endif

//Use of LED matrix display
#define USE_LED_SCREEN 1

//Use of Coin Acceptor
#define USE_COIN_ACCEPTOR 0

//GAME VARIABLES
#define GAME_TIME 30 
//DEBUG MORE
#define DEBUG 0 //if set to 1 will print messages on Serial port.

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may 
// need to be adapted
#define	MAX_DEVICES	4
#define	MAX_ZONES	  4
#define UP_MAX_ZONES 2
#define ZONE_SIZE (MAX_DEVICES/MAX_ZONES)   // integer multiple works best

//Connetion PINS for LED matrix
#define	CLK_PIN		13
#define	DATA_PIN	11
#define	CS_PIN		10

// Hardware SPI connection
MD_Parola P = MD_Parola(CS_PIN, MAX_DEVICES);
// Arbitrary output pins
// MD_Parola P = MD_Parola(DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Global variables
int btn_pins[] = {3,5,7,9};
int led_pins[] = {2,4,6,8};
const int MAX_CONNS = 4;
int buzz_minus = A0;
int buzz_plus = A3;
int countdown=GAME_TIME;
int score = 0; //Loop adds one point first time it runs
bool game_on = false;

#if USE_LED_SCREEN
//Digit variables for LED matrix
int d1 = 0;
int d2 = 0;
int d3 = 0;
int d4 = 0;
//Used for countdown
unsigned long currentMillis = 0;
const long interval = 1000; 
unsigned long previousMillis = 0;
#endif

int idx = -1;

void setup(void) {

  Serial.begin(9600);

#if USE_LED_SCREEN
  P.begin(MAX_ZONES);
  P.setInvert(false);
  P.setIntensity(8);
  for (uint8_t i=0; i<MAX_ZONES; i++)
  {
    P.setZone(i, ZONE_SIZE*i, (ZONE_SIZE*(i+1))-1);
  }
#endif

  for(int i=0; i<MAX_CONNS; i++) {
    pinMode(led_pins[i], OUTPUT);
    pinMode(btn_pins[i], INPUT_PULLUP);
  }
  
  // Set up the pins for the buzzer:
  pinMode(buzz_plus, OUTPUT);     
  pinMode(buzz_minus, OUTPUT);     
  digitalWrite(buzz_minus, LOW);    // pull the minus pin to 0 Volt by making it "LOW"
  Serial.println("Game starting...");
  
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
        AddPoint();
        generateRandom();
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

void gameStart() {
  game_on = true;
  //Initialize global vars
  score = 0;
  countdown = GAME_TIME;
  //Generate First random number
  generateRandom();
  //
  #if USE_LED_SCREEN
    P.displayZoneText(0,"0",CENTER,5, 0,PRINT, NO_EFFECT);
    P.displayZoneText(1,"0",CENTER,5, 0,PRINT, NO_EFFECT);  
    P.displayAnimate();
    delay(500);
  #endif
  
  for(int i=0; i<MAX_CONNS; i++) {
    digitalWrite(led_pins[i], HIGH);    
  }
}

void gameEnd() {
  game_on = false;
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

