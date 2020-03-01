#include "HX711.h"
#include <MFRC522.h>
#define SS_PIN 53
#define RST_PIN 5

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
//HX711 loadcell;

// Balance
int sck_pin = 22;
int din_pin = 24;

// Motor
int motor_pin = A0;

// Buttons
int button_pin = 49;

int turns = 10; // how fast the motor runs
int turnAmount = 30; // how many turns the motor makes
unsigned long currentTime;
unsigned long timer;
unsigned long loopTime;

int buttonPressed;
bool tmpButtonPressed = false;

const long LOADCELL_OFFSET = 50682624;
const long LOADCELL_DIVIDER = 5895655;

const int LOADCELL_DOUT_PIN = 24;
const int LOADCELL_SCK_PIN = 22;
Hx711 scale(din_pin, sck_pin);
String latestCardId = "";
String cardId = "";

//Hx711 loadcell;
void setup()
{
  Serial.begin(57600);    // Initialize serial communications with the PC
  while (!Serial);    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see
  pinMode(button_pin, OUTPUT);
  /*
      pinMode(motor_pin, OUTPUT);
      currentTime = millis();
      timer = millis();
  */
  currentTime = millis();
  loopTime = currentTime;
}

void loop()
{
  currentTime = millis();
  buttonPressed = digitalRead(button_pin);   // read the input pin

  if (latestCardId != cardId) {
    Serial.println("Nouveau client");
    latestCardId = cardId;

  }
  if (buttonPressed == false) {
    analogWrite(motor_pin, 0);
    //Serial.println(scale.getGram(), 1);
  }

  if (tmpButtonPressed == true && buttonPressed == false) {
    tmpButtonPressed = false;
    Serial.println(scale.getGram(), 1);
  }

  if (buttonPressed) {
    tmpButtonPressed = true;
  }

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent()) {
    analogWrite(motor_pin, 0);
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  byte letter;

  cardId = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    //Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    //Serial.print(mfrc522.uid.uidByte[i], HEX);
    cardId.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    cardId.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  if (cardId != "" && buttonPressed) {
    if (currentTime >= (loopTime + 20)) {
      analogWrite(motor_pin, turns);
      turns = turns + turnAmount;
      loopTime = currentTime; // Updates loopTime
    }
  }

  if (buttonPressed == false) {
    analogWrite(motor_pin, 0);
  }

  /*
    currentTime = millis();
    buttonPressed = digitalRead(button_pin);   // read the input pin

    if (currentTime >= (loopTime + 20)) {
      analogWrite(motor_pin, turns);
      turns = turns + turnAmount;
      loopTime = currentTime; // Updates loopTime
    }
    } else {
    analogWrite(motor_pin, 0);
    }
  */
}
