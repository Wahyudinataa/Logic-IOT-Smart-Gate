#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LCD_ADDRESS 0x27

const int TRIG_PINS[4] = {14, 27, 26, 25};
const int ECHO_PINS[4] = {17, 16, 2, 15};

const int MAX_PARKING_SLOTS = 4;
const int DISTANCE_THRESHOLD = 5; // cm
const char SLOT_LABELS[4] = {'A', 'B', 'C', 'D'};

const unsigned long SLOT_SCAN_INTERVAL = 2000;

struct ParkingSlotInfo {
  char label;
  bool isEmpty;
  int distance;
  unsigned long lastChecked;
};

LiquidCrystal_I2C display(LCD_ADDRESS, 16, 2);

ParkingSlotInfo parkingSlots[MAX_PARKING_SLOTS];
int availableSlots = MAX_PARKING_SLOTS;


unsigned long lastSlotScan = 0;

void initializeSystem();
void setupDisplayScreen();
void setupSensorPins();
void initializeParkingSlots();
void scanParkingSlotsStatus();
int measureSlotDistance(int slotIndex);
int countEmptySlots();
void logSlotStatusChange(int slotIndex, bool newStatus);
bool checkTimer(unsigned long& lastTime, unsigned long interval);

void setup() {
  Serial.begin(115200);
  Serial.println("=== Sistem Parkir Motor (Logika Fisik) ===");

  initializeSystem();

  Serial.println("=== Sistem Siap ===");
  delay(2000);
}

void loop() {
  scanParkingSlotsStatus();
  delay(50); 
}

void initializeSystem() {
  display.init();
  display.backlight();
  setupDisplayScreen();
  setupSensorPins();
  initializeParkingSlots();
}

void setupDisplayScreen() {
  display.clear();
  display.setCursor(0, 0);
  display.print("Parkir Motor");
  display.setCursor(0, 1);
  display.print("Inisialisasi...");
  delay(2000);
}

void setupSensorPins() {
  for (int i = 0; i < MAX_PARKING_SLOTS; i++) {
    pinMode(TRIG_PINS[i], OUTPUT);
    pinMode(ECHO_PINS[i], INPUT);
  }

  Serial.println("Sensor Ultrasonik Siap");
}

void initializeParkingSlots() {
  Serial.println("=== Status Awal Slot ===");
  int occupiedSlots = 0;

  for (int i = 0; i < MAX_PARKING_SLOTS; i++) {
    parkingSlots[i].label = SLOT_LABELS[i];
    parkingSlots[i].distance = measureSlotDistance(i);
    parkingSlots[i].isEmpty = (parkingSlots[i].distance > DISTANCE_THRESHOLD);
    parkingSlots[i].lastChecked = millis();

    if (!parkingSlots[i].isEmpty) {
      occupiedSlots++;
    }

    Serial.printf("Slot %c: %s (Jarak: %d cm)\n",
                  parkingSlots[i].label,
                  parkingSlots[i].isEmpty ? "KOSONG" : "TERISI",
                  parkingSlots[i].distance);
  }

  availableSlots = MAX_PARKING_SLOTS - occupiedSlots;
  Serial.printf("Slot Kosong Saat Mulai: %d/%d\n", availableSlots, MAX_PARKING_SLOTS);
}

void scanParkingSlotsStatus() {
  if (!checkTimer(lastSlotScan, SLOT_SCAN_INTERVAL)) return;

  bool statusChanged = false;

  for (int i = 0; i < MAX_PARKING_SLOTS; i++) {
    int currentDistance = measureSlotDistance(i);
    bool newEmptyStatus = (currentDistance > DISTANCE_THRESHOLD);

    if (parkingSlots[i].isEmpty != newEmptyStatus) {
      logSlotStatusChange(i, newEmptyStatus);
      parkingSlots[i].isEmpty = newEmptyStatus;
      statusChanged = true;
    }

    parkingSlots[i].distance = currentDistance;
    parkingSlots[i].lastChecked = millis();
  }

  if (statusChanged) {
    int newAvailable = countEmptySlots();
    if (newAvailable != availableSlots) {
      availableSlots = newAvailable;
      Serial.printf("Slot Kosong Sekarang: %d/%d\n", availableSlots, MAX_PARKING_SLOTS);
    }
  }
}

int measureSlotDistance(int slotIndex) {
  digitalWrite(TRIG_PINS[slotIndex], LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PINS[slotIndex], HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PINS[slotIndex], LOW);

  long duration = pulseIn(ECHO_PINS[slotIndex], HIGH, 30000);
  if (duration == 0) duration = 30000; // Timeout handling

  return (duration * 0.034) / 2;
}

int countEmptySlots() {
  int count = 0;
  for (int i = 0; i < MAX_PARKING_SLOTS; i++) {
    if (parkingSlots[i].isEmpty) count++;
  }
  return count;
}

void logSlotStatusChange(int slotIndex, bool newStatus) {
  Serial.printf("Slot %c berubah: %s -> %s (Jarak: %d cm)\n",
                parkingSlots[slotIndex].label,
                parkingSlots[slotIndex].isEmpty ? "KOSONG" : "TERISI",
                newStatus ? "KOSONG" : "TERISI",
                parkingSlots[slotIndex].distance);
}

bool checkTimer(unsigned long& lastTime, unsigned long interval) {
  if (millis() - lastTime >= interval) {
    lastTime = millis();
    return true;
  }
  return false;
}
