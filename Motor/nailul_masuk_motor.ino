#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

#define SS_PIN 5
#define RST_PIN 4
MFRC522 rfid(SS_PIN, RST_PIN);

LiquidCrystal_I2C lcd(0x27, 16, 2);

Servo servoMotor;
Servo servoMotor2;

#define trigPin1 18 // Ultrasonik slot 1
#define echoPin1 19
#define trigPin2 21 // Ultrasonik slot 2
#define echoPin2 22

int slot1 = 0;
int slot2 = 0;

bool slotAvailable = false;
int slotPilihan = 0; // 1 = motor, 2 = mobil

// Ganti dengan UID kartu RFID yang valid
byte validUIDs[][4] = {
  {0xDE, 0xAD, 0xBE, 0xEF}, // contoh UID 1
  {0x12, 0x34, 0x56, 0x78}  // contoh UID 2
};

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  lcd.init();
  lcd.backlight();
  lcd.clear();

  servoMotor.attach(13);  // Servo motor motor
  servoMotor2.attach(12); // Servo motor mobil

  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);

  servoMotor.write(0);
  servoMotor2.write(0);

  lcd.setCursor(0, 0);
  lcd.print(" Smart Parking ");
  delay(2000);
  lcd.clear();
}

void loop() {
  checkSlot(); // Periksa ketersediaan slot

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  if (isValidUID(rfid.uid.uidByte)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kartu Dikenali");

    if (slot1 == 0) {
      slotPilihan = 1;
      slotAvailable = true;
    } else if (slot2 == 0) {
      slotPilihan = 2;
      slotAvailable = true;
    } else {
      slotAvailable = false;
    }

    if (slotAvailable) {
      lcd.setCursor(0, 1);
      lcd.print("Slot Tersedia");
      bukaGerbangMasuk(slotPilihan);
    } else {
      lcd.setCursor(0, 1);
      lcd.print("Penuh!");
    }
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kartu Tidak");
    lcd.setCursor(0, 1);
    lcd.print("Dikenali");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(2000);
}

bool isValidUID(byte *uid) {
  for (int i = 0; i < sizeof(validUIDs) / sizeof(validUIDs[0]); i++) {
    bool match = true;
    for (int j = 0; j < 4; j++) {
      if (uid[j] != validUIDs[i][j]) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}

void bukaGerbangMasuk(int slot) {
  if (slot == 1) {
    servoMotor.write(90);
    delay(3000); // waktu lewat delay
    servoMotor.write(0);
    slot1 = 1;
  } else if (slot == 2) {
    servoMotor2.write(90);
    delay(3000);
    servoMotor2.write(0);
    slot2 = 1;
  }
}

void checkSlot() {
  long duration1, distance1;
  long duration2, distance2;

  digitalWrite(trigPin1, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);
  duration1 = pulseIn(echoPin1, HIGH);
  distance1 = duration1 * 0.034 / 2;
  if (distance1 < 10) slot1 = 1; else slot1 = 0;

  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);
  duration2 = pulseIn(echoPin2, HIGH);
  distance2 = duration2 * 0.034 / 2;
  if (distance2 < 10) slot2 = 1; else slot2 = 0;
}