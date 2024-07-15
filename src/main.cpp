#include <Arduino.h>
#include <Wire.h>
#include <ezButton.h>
#include <RotaryEncoder.h>
#include <Adafruit_SSD1306.h>

#include <MIDI.h>

#define MIDIinPin 0
#define MIDIoutPin 1

#define screenSDA A4 //OLED i2C SDA Pin
#define screenSCL A5 //OLED i2C SCL 

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define pinAKY  2  //CLK Output A of the encoder
#define pinBKY  3 //DT Output B of the encoder
ezButton button1(4); // switch of the encoder

#define tapPin 5

#define optoPin 6

RotaryEncoder encoder(pinAKY, pinBKY, RotaryEncoder::LatchMode::FOUR3); 

int modeIndex = 1;
bool ioState = false;

int global_bpm = 80;
int newBpm = 60;
// ARDUINO CODE

void setup() {

  //init de l'interruption i/O
  attachInterrupt(digitalPinToInterrupt(tapPin), ioButton, RISING);

  //init du display
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
      Serial.println("SSD1306 allocation failed");
      for(;;); // Don't proceed, loop forever
    }
    display.clearDisplay();
    delay(2000);
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(30, 16);
    // Display static text
    display.println("TAP");
    display.setTextSize(2);
    display.setCursor(16, 42);
    display.println("is ready!");
    display.display();
    delay(1000);

  //init button Encoder
  button1.setDebounceTime(50);

  pinMode(optoPin,OUTPUT);

}

// INTERRUPTIONS

void ioButton(){

  switch (modeIndex) {
            case 0:
                digitalWrite(optoPin,0);
                delay(5);
                digitalWrite(optoPin,1);
                break;
            case 1:

                break;
            case 2:

                break;
    }
}

void loop() {

 button1.loop(); // Met à jour l'état du bouton

    if (button1.isPressed()) {
        modeIndex = (modeIndex + 1) % 3;  // Changer de mode

        switch (modeIndex) {
            case 0:
// Affiche Manual à l'écran
                break;
            case 1:
// Affiche Master à l'écran
                break;
            case 2:
//Affiche lave à l'écran
                break;
        }
        //delay(2000); à faire avec millis ! 
//Retourne à l'affichage du tempo
    }

    // rotary switch KY040
    static int pos = 0;
    encoder.tick();

    int newPos = encoder.getPosition();
    if (pos != newPos) {

        newBpm += (float)encoder.getDirection();

        
        Serial.print("New Bpm = ");
        Serial.println(newBpm);
        

        global_bpm = newBpm;
        //Affiche le tempo (pas de delay, prend le relais si un menu est affiché)
      pos = newPos;
    } // if



}

