// Sistem Parkir Motor Sederhana - Hanya Logika Masuk dan Slot

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// ================================
// KONFIGURASI PIN DAN KONSTANTA
// ================================
#define RFID_SS_PIN 5
#define RFID_RST_PIN 4
#define SERVO_ENTRY_PIN 13
#define LCD_ADDRESS 0x27

// Pin Ultrasonik untuk 4 slot motor
const int TRIG_PINS[4] = {14, 27, 26, 25};
const int ECHO_PINS[4] = {17, 16, 2, 15};

const int MAX_PARKING_SLOTS = 4;
const int DISTANCE_THRESHOLD = 5; // cm
const char SLOT_LABELS[4] = {'A', 'B', 'C', 'D'};

const unsigned long SLOT_SCAN_INTERVAL = 2000;
const unsigned long DISPLAY_UPDATE_INTERVAL = 4000;
const unsigned long GATE_OPEN_TIME = 8000;

// ================================
// STRUKTUR DATA
// ================================
struct StudentData {
  String name;
  String nim;
  String faculty;
  String status;
  bool isValid;

  void clear() {
    name = "";
    nim = "";
    faculty = "";
    status = "";
    isValid = false;
  }
};

struct ParkingSlotInfo {
  char label;
  bool isEmpty;
  int distance;
  unsigned long lastChecked;
};

struct SystemTimers {
  unsigned long lastSlotScan;
  unsigned long lastDisplayUpdate;
  unsigned long gateTimer;
};

struct SystemFlags {
  bool displayToggle;
  bool gateEntryOpen;
  bool processingCard;
};

// ================================
// VARIABEL GLOBAL
// ================================
MFRC522 rfidReader(RFID_SS_PIN, RFID_RST_PIN);
MFRC522::MIFARE_Key rfidKey;
LiquidCrystal_I2C display(LCD_ADDRESS, 16, 2);
Servo entryGate;

ParkingSlotInfo parkingSlots[MAX_PARKING_SLOTS];
StudentData currentStudent;
SystemTimers timers;
SystemFlags flags;
int availableSlots = MAX_PARKING_SLOTS;

// ================================
// SETUP
// ================================
void setup() {
  Serial.begin(115200);
  initializeSystem();
  Serial.println("=== Sistem Parkir Motor Siap ===");
}

// ================================
// LOOP UTAMA
// ================================
void loop() {
  scanParkingSlotsStatus();
  updateDisplayInformation();
  processRFIDCardDetection();
  manageGateOperations();
  delay(50);
}

// ================================
// INISIALISASI SISTEM
// ================================
void initializeSystem() {
  display.init();
  display.backlight();

  SPI.begin();
  rfidReader.PCD_Init();
  for (byte i = 0; i < 6; i++) rfidKey.keyByte[i] = 0xFF;

  entryGate.attach(SERVO_ENTRY_PIN);
  entryGate.write(0);

  for (int i = 0; i < MAX_PARKING_SLOTS; i++) {
    pinMode(TRIG_PINS[i], OUTPUT);
    pinMode(ECHO_PINS[i], INPUT);
  }

  for (int i = 0; i < MAX_PARKING_SLOTS; i++) {
    parkingSlots[i].label = SLOT_LABELS[i];
    parkingSlots[i].distance = measureSlotDistance(i);
    parkingSlots[i].isEmpty = (parkingSlots[i].distance > DISTANCE_THRESHOLD);
    parkingSlots[i].lastChecked = millis();
  }

  memset(&timers, 0, sizeof(timers));
  memset(&flags, 0, sizeof(flags));
  currentStudent.clear();

  displayMessage("Smart Parking", "System Ready", 2000);
}

// ================================
// PEMINDAIAN SLOT PARKIR
// ================================
void scanParkingSlotsStatus() {
  if (millis() - timers.lastSlotScan < SLOT_SCAN_INTERVAL) return;
  timers.lastSlotScan = millis();

  int count = 0;
  for (int i = 0; i < MAX_PARKING_SLOTS; i++) {
    int d = measureSlotDistance(i);
    parkingSlots[i].isEmpty = (d > DISTANCE_THRESHOLD);
    parkingSlots[i].distance = d;
    if (parkingSlots[i].isEmpty) count++;
  }
  availableSlots = count;
}

int measureSlotDistance(int index) {
  digitalWrite(TRIG_PINS[index], LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PINS[index], HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PINS[index], LOW);

  long duration = pulseIn(ECHO_PINS[index], HIGH, 30000);
  if (duration == 0) duration = 30000;

  return (duration * 0.034) / 2;
}

// ================================
// PEMROSESAN KARTU RFID
// ================================
void processRFIDCardDetection() {
  if (flags.processingCard || flags.gateEntryOpen) return;

  if (rfidReader.PICC_IsNewCardPresent() && rfidReader.PICC_ReadCardSerial()) {
    flags.processingCard = true;

    // Untuk contoh, semua kartu dianggap valid mahasiswa aktif
    currentStudent.name = "Nama Mahasiswa";
    currentStudent.nim = "1234567890";
    currentStudent.faculty = "FTIK";
    currentStudent.status = "AKTIF";
    currentStudent.isValid = true;

    displayMessage("Selamat Datang", currentStudent.name, 2000);

    if (currentStudent.status == "AKTIF" && availableSlots > 0) {
      char slotLabel = findFirstEmptySlot();
      displayMessage("Silakan Parkir", String("Slot: ") + slotLabel, 2000);
      openEntryGateSequence();
    } else {
      displayMessage("Parkir Penuh", "Coba Lagi", 2000);
    }

    rfidReader.PICC_HaltA();
    rfidReader.PCD_StopCrypto1();
    currentStudent.clear();
    flags.processingCard = false;
  }
}

char findFirstEmptySlot() {
  for (int i = 0; i < MAX_PARKING_SLOTS; i++) {
    if (parkingSlots[i].isEmpty) return parkingSlots[i].label;
  }
  return '-';
}

// ================================
// PENGATURAN GERBANG MASUK
// ================================
void openEntryGateSequence() {
  displayMessage("Gate Masuk", "Dibuka", 1500);
  entryGate.write(90);
  flags.gateEntryOpen = true;
  timers.gateTimer = millis();
}

void manageGateOperations() {
  if (flags.gateEntryOpen && millis() - timers.gateTimer > GATE_OPEN_TIME) {
    entryGate.write(0);
    displayMessage("Gate Masuk", "Tertutup", 1500);
    flags.gateEntryOpen = false;
  }
}

// ================================
// DISPLAY LCD
// ================================
void updateDisplayInformation() {
  if (flags.processingCard || flags.gateEntryOpen) return;
  if (millis() - timers.lastDisplayUpdate < DISPLAY_UPDATE_INTERVAL) return;

  timers.lastDisplayUpdate = millis();
  display.clear();
  display.setCursor(0, 0);
  display.print("Slot Kosong: ");
  display.print(availableSlots);
  display.setCursor(0, 1);
  display.print("Tempelkan KTM");
}

void displayMessage(String l1, String l2, int durasi) {
  display.clear();
  display.setCursor(0, 0);
  display.print(l1);
  display.setCursor(0, 1);
  display.print(l2);
  delay(durasi);
}