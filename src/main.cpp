#include <Arduino.h>
#include <EEPROM.h>

#include <Wire.h>
#include <ezButton.h>
#include <RotaryEncoder.h>
#include <Adafruit_SSD1306.h>

#include <stdio.h>

#include <MIDI.h>

#define MIDIinPin 0
#define MIDIoutPin 1

#define screenSDA 2 //OLED i2C SDA Pin
#define screenSCL 3 //OLED i2C SCL 

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define pinAKY  15  //CLK Output A of the encoder
#define pinBKY  14 //DT Output B of the encoder
ezButton button1(16); // switch of the encoder

#define tapPin 4

#define optoPin 7

#define BIG_LED 

RotaryEncoder encoder(pinAKY, pinBKY, RotaryEncoder::LatchMode::FOUR3); 


bool ioState = false;

const int eepromBpmAddress = 0;
const byte eepromModeAddress = sizeof(byte);

volatile unsigned int globalBpm;
unsigned int newBpm;

volatile byte modeIndex;

unsigned int clockCount;
unsigned int clockCountMax;

byte barCount = 0;
bool barFlag = 0;

unsigned int gateTime = 0; //ms
unsigned int gateTimeMax = 5; //ms
bool gateState = 0;

unsigned int waitTime = 0; //ms
unsigned int waitTimeMax = 2000; //ms

void ioButton();
void displayTempo(unsigned int tempo);
void displayMode(String mode);

void setup() {

  //Recal the EEPROM Memory for tempo

  EEPROM.get(eepromBpmAddress, globalBpm);
  if (globalBpm == 0xFFFFFFFF || globalBpm == 0) {
      globalBpm = 80;
  } //if
  newBpm = globalBpm;

  EEPROM.get(eepromModeAddress, modeIndex);
  if (modeIndex == 0xFF || modeIndex < 0 || modeIndex > 2) {
    modeIndex = 0;
  }
  cli(); //disable all interruptions

  //init timer interruption
  TCCR1A = 0;               // Set entire TCCR1A register to 0
  TCCR1B = 0;               // Same for TCCR1B
  TCNT1 = 0;                // Initialise le compteur à 0

  // Configure le compare match register pour générer une interruption toutes les 0.1ms
  OCR1A = 159;              // 160 (16 MHz / (8 * 1250)) - 1

  // Configure le mode CTC (Clear Timer on Compare Match)
  TCCR1B |= (1 << WGM12);

  // Configure le prescaler sur 8
  TCCR1B |= (1 << CS11);

  // Activer l'interruption sur compare match A
  TIMSK1 |= (1 << OCIE1A);

  sei();

  //init de l'interruption i/O
  attachInterrupt(digitalPinToInterrupt(tapPin), ioButton, RISING);

  //init button Encoder
  button1.setDebounceTime(50);

  //init des digital outputs
  pinMode(optoPin,OUTPUT);
  pinMode(LED_BUILTIN,OUTPUT);
  
  //init du display
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
      //migth be SSD1306_SWITCHCAPVCC
      Serial.println("SSD1306 allocation failed");
      for(;;); // Don't proceed, loop forever
    } //if
    display.clearDisplay();
    delay(2000);
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(30, 16);
    // Display static text
    display.println("TAP is Ready");
    display.display();
    delay(1000);
} // setup

// INTERRUPTIONS

void ioButton(){

  switch (modeIndex) {
            case 0: // Trigger 4 times and stop
                barCount =0;
                barFlag = 1;
                break;
            case 1: //Read the tapped Tempo
                break;
            case 2: //Desactivated

                break;
    } //switch
} //ioButton

ISR(TIMER1_COMPA_vect) {

  if (clockCount++ > clockCountMax) { // clockCount = durée d'un pas temporel en 0,1ms
    clockCount = 0;
    digitalWrite(LED_BUILTIN, HIGH);

    switch (modeIndex){
    case 0:
      if (barFlag){ //Only on mode 0
          if (barCount++ < 4) { //opens the gate 4 times while barFlag is true
            digitalWrite(optoPin, HIGH);
            gateState = 1;
            gateTime = millis();
          }// if
          else{
            barFlag = 0;
            barCount = 0;
          }// else
      } //if
      break;
    default: // mode 1 or 2
      digitalWrite(optoPin, HIGH);
      gateState = 1;
      gateTime = 0;
      break;
    }// switch 
  }  

}

void loop() {

  static  unsigned int ms = millis();
  if (ms >= gateTime + gateTimeMax) { //close the gate after 5ms
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(optoPin, LOW);
    gateState = 0;
    gateTime = 0;
  } //if

 button1.loop(); // Met à jour l'état du bouton

    if (button1.isPressed()) {
      modeIndex = (modeIndex + 1) % 3;  // Changer de mode
      EEPROM.put(eepromModeAddress, modeIndex);
      switch (modeIndex) {
          case 0:
// Affiche Trigger
              displayMode("Trigger Mode");
              break;
          case 1:
// Affiche Synced
              displayMode("Synced Mode");
              break;
          case 2:
//Affiche MIDI in
              displayMode("MIDI in Mode");
              break;
      }
    } // if

    // rotary switch KY040
    static int pos = 0;
    encoder.tick();

    int newPos = encoder.getPosition();

    if (pos != newPos) {

        newBpm += (float)encoder.getDirection();

        if (newBpm > 40 || newBpm < 220) {

        Serial.print("New Bpm = ");
        Serial.println(newBpm);

        globalBpm = newBpm;
        EEPROM.put(eepromBpmAddress, globalBpm); //écrit la valeur dans la mémoire EEPROM

        clockCountMax = (600000 / globalBpm); // 60 000 / bpm = durée d'un pas temporel en ms
        }
        else if (newBpm < 40) {
          newBpm = 40;
        }
        else if (newBpm > 220) {
          newBpm = 220;
        }
      pos = newPos;
    } // if

    static unsigned wait = millis();
    if ( wait >= waitTime + waitTimeMax) { //close the gate after 2000 ms
      displayTempo(globalBpm);
    }

}

void displayTempo(unsigned int tempo) {
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(30, 16);
  display.println(tempo);
  display.display();
}


void displayMode(String mode) {
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(WHITE);
  display.setCursor(30, 16);
  display.println(mode);
  display.display();
}