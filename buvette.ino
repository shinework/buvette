#include "HX711.h"
#include <MFRC522.h>
#include <TimerOne.h>
#define SS_PIN 53
#define RST_PIN A3
// #define RST_PIN 5
#define DEBUG 0
#define MOTOR_ONLY 1












MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
MFRC522::MIFARE_Key key;

// Balance
// int sck_pin = 22;
// int din_pin = 24;
int sck_pin = A1;
int din_pin = A2;
float startScale = 0;
float stopScale = 0;

// Motor
int motor_pin = A0;

// Buttons
int button_pin = 49;

int turns = 10; // how fast the motor runs
int turnAmount = 30; // how many turns the motor makes

unsigned long currentTime;
unsigned long timer;
unsigned long loopTime;

bool isButtonPressed;
bool didButtonPressed = false;
bool firstLoop = true;

const long LOADCELL_OFFSET = 50682624;
const long LOADCELL_DIVIDER = 5895655;

const int LOADCELL_DOUT_PIN = 24;
const int LOADCELL_SCK_PIN = 22;
Hx711 scale(din_pin, sck_pin);

String prevLoopCardId = "";
String cardId = "";

void setup()
{
  //Timer1.initialize(9000000); // 9 second
  //Timer1.attachInterrupt(getScale);
  
  Serial.begin(57600);
  while (!Serial);
  SPI.begin();
  mfrc522.PCD_Init();

  // Prepare the key
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  pinMode(button_pin, INPUT_PULLUP);
  loopTime = currentTime;

  if (!MOTOR_ONLY) {
    startScale = scale.getGram();
  }
}

void stopMotor() {
  analogWrite(motor_pin, 0);
}

void runMotor() {
  analogWrite(motor_pin, turns);
  turns = turns + turnAmount;
  loopTime = currentTime; // Updates loopTime
}

String getCardId() {
  byte letter;

  String id = "";
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    //Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    //Serial.print(mfrc522.uid.uidByte[i], HEX);
    id.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    id.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  return id;
}

void getScale() {
  if (!isButtonPressed) {
    startScale = scale.getGram();
    Serial.println("prise pesée");
    Serial.println(startScale);
  }
}

void handleStart() {
  if (!didButtonPressed && isButtonPressed) {
    Serial.println("Début pesée:");
    //startScale = scale.getGram();
    Serial.println(startScale);
  }
}

void handleStop() {
  if (didButtonPressed && !isButtonPressed) {
    stopScale = scale.getGram();
    float measure = (startScale - stopScale) * 19.28;

    Serial.println("Grammes à facturer :");
    Serial.println(stopScale, 1);
    Serial.println(measure, 1);
    writeMeasure(measure);
    startScale = stopScale;
  }
}

void resetCard() {
  byte sector         = 1;
  byte blockAddr      = 4;
  byte trailerBlock   = 7;

  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.println(F("PCD_Authenticate() failed: "));
    return;
  }

  // Read
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }

  byte dataBlock[]    = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
  };

  // Write data to the block
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }

  Serial.println("Carte réinitialisée:");
  mfrc522.PCD_StopCrypto1();
}

void writeMeasure(float measure)
{
  byte sector         = 1;
  byte blockAddr      = 4;
  byte trailerBlock   = 7;

  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.println(F("PCD_Authenticate() failed: "));
    return;
  }

  // Read
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }

  String currentMeasureString = (char*)buffer;

  float newMeasure = currentMeasureString.toFloat() + measure;
  String newMeasureString = String(newMeasure);
  byte dataBlock[]    = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
  };
  newMeasureString.getBytes(dataBlock, newMeasureString.length());

  // Write data to the block
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }

  //Serial.print("Measure writed in RFID card");
  Serial.println("Nouvelle quantité enregistrée sur la carte:");
  Serial.println(newMeasureString);
  
  mfrc522.PCD_StopCrypto1();
}

void readMeasure()
{
  byte sector         = 1;
  byte blockAddr      = 4;
  byte trailerBlock   = 7;

  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    Serial.println(F("PCD_Authenticate() failed: "));
    return;
  }

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }

  String measure = (char*)buffer;
  Serial.println("Quantité actuelle sur la carte:");
  Serial.println(measure);

  mfrc522.PCD_StopCrypto1();
}

void loop()
{
  isButtonPressed = digitalRead(button_pin) == LOW; 
  currentTime = millis();

  if (MOTOR_ONLY) {
    isButtonPressed = digitalRead(button_pin) == LOW;
    if (!isButtonPressed) {
      stopMotor();
    }

    if (isButtonPressed && currentTime >= (loopTime + 20)) {
      runMotor();
    }
  
    return;
  }

  if (!mfrc522.PICC_IsNewCardPresent()) {
    stopMotor();
    return;
  }
  
  if (!mfrc522.PICC_ReadCardSerial()) {
    stopMotor();
    return;
  }
  
  if (firstLoop) {
    //resetCard();
    readMeasure();
    firstLoop = false;
  }

  isButtonPressed = digitalRead(button_pin) == LOW;
 
  if (prevLoopCardId != cardId) {
    prevLoopCardId = cardId;
  }

  if (!isButtonPressed) {
    stopMotor();
  }

  //handleStart();
  handleStop();

  cardId = getCardId();

  if (cardId != "" && isButtonPressed && currentTime >= (loopTime + 20)) {
    runMotor();
  }

  didButtonPressed = isButtonPressed;
}

void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
