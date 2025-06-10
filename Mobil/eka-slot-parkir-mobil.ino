#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Ultrasonic sensor pins
const int trigPins[] = {14, 27, 26, 25};
const int echoPins[] = {17, 16, 2, 15};
const int slotCount = 4;
const char slotNames[] = {'A', 'B', 'C', 'D'};

// SISTEM TRACKING PARKIR
int slotTersedia = 4;
int maxKapasitasParkir = 4;
bool statusSlotFisik[4];

// Variables untuk timing
unsigned long lastSlotCheck = 0;
unsigned long lastDisplayUpdate = 0;
bool showDetailDisplay = false;

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Slot");
  lcd.setCursor(0, 1);
  lcd.print("Parkir Ready");
  
  // Inisialisasi Ultrasonic sensors
  initializeUltrasonicSensors();
  
  // Inisialisasi status slot parkir
  inisialisasiSlotParkir();
  
  delay(2000);
}

void loop() {
  // 1. Update ketersediaan parkir - Update status fisik slot
  updateKetersediaanParkir();
  
  // 2. Tampilkan informasi slot mobil
  tampilkanInfoSlotMobil();
  
  delay(100); // Delay kecil untuk stabilitas
}

// ===============================
// LOGIKA KETERSEDIAAN PARKIR
// ===============================

void updateKetersediaanParkir() {
  // Update status fisik slot setiap 2 detik
  if (millis() - lastSlotCheck > 2000) {
    for (int i = 0; i < slotCount; i++) {
      int jarak = getJarakUltrasonik(i);
      bool statusBaru = (jarak > 5); // true = kosong, false = terisi
      
      // Log perubahan status fisik
      if (statusSlotFisik[i] != statusBaru) {
        Serial.println("FISIK - Slot " + String(slotNames[i]) + " berubah: " + 
                      (statusBaru ? "KOSONG" : "TERISI") + " (Jarak: " + String(jarak) + " cm)");
      }
      
      statusSlotFisik[i] = statusBaru;
    }
    
    // Sinkronisasi tracking dengan kondisi fisik
    slotTersedia = hitungSlotFisikKosong();
    
    lastSlotCheck = millis();
    
    // Debug info ketersediaan
    Serial.println("KETERSEDIAAN - Slot Tersedia: " + String(slotTersedia) + "/" + String(maxKapasitasParkir));
  }
}

int hitungSlotFisikKosong() {
  int kosong = 0;
  for (int i = 0; i < slotCount; i++) {
    if (statusSlotFisik[i]) kosong++;
  }
  return kosong;
}

bool cekKetersediaanParkir() {
  return (slotTersedia > 0);
}

// ===============================
// LOGIKA TAMPILAN SLOT MOBIL
// ===============================

void tampilkanInfoSlotMobil() {
  // Update display setiap 4 detik
  if (millis() - lastDisplayUpdate > 4000) {
    lcd.clear();
    
    if (!showDetailDisplay) {
      // Tampilan tracking sistem
      tampilkanInfoTracking();
    } else {
      // Tampilan detail slot fisik
      tampilkanDetailSlot();
    }
    
    showDetailDisplay = !showDetailDisplay;
    lastDisplayUpdate = millis();
  }
}

void tampilkanInfoTracking() {
  lcd.setCursor(0, 0);
  lcd.print("Slot Tersedia:");
  lcd.print(slotTersedia);
  lcd.print("/");
  lcd.print(maxKapasitasParkir);
  
  lcd.setCursor(0, 1);
  if (cekKetersediaanParkir()) {
    lcd.print("Ada Tempat Kosong");
  } else {
    lcd.print("Area Penuh!");
  }
}

void tampilkanDetailSlot() {
  String baris1 = "";
  String baris2 = "";
  
  for (int i = 0; i < slotCount; i++) {
    if (i < 2) {
      baris1 += String(slotNames[i]) + ":";
      baris1 += statusSlotFisik[i] ? "K " : "T ";
    } else {
      baris2 += String(slotNames[i]) + ":";
      baris2 += statusSlotFisik[i] ? "K " : "T ";
    }
  }
  
  lcd.setCursor(0, 0);
  lcd.print(baris1);
  lcd.setCursor(0, 1);
  lcd.print(baris2);
}

char cariSlotKosongPertama() {
  for (int i = 0; i < slotCount; i++) {
    if (statusSlotFisik[i]) return slotNames[i];
  }
  return ' '; // Tidak ada slot kosong
}

// ===============================
// FUNGSI UTILITAS
// ===============================

int getJarakUltrasonik(int slot) {
  digitalWrite(trigPins[slot], LOW);
  delayMicroseconds(2);
  digitalWrite(trigPins[slot], HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPins[slot], LOW);
  
  long durasi = pulseIn(echoPins[slot], HIGH, 30000);
  if (durasi == 0) durasi = 30000; // Timeout
  return durasi * 0.034 / 2;
}

void printStatusSlot() {
  Serial.println("\n=== STATUS SLOT PARKIR ===");
  for (int i = 0; i < slotCount; i++) {
    int jarak = getJarakUltrasonik(i);
    String status = statusSlotFisik[i] ? "KOSONG" : "TERISI";
    Serial.println("Slot " + String(slotNames[i]) + ": " + status + " (Jarak: " + String(jarak) + " cm)");
  }
  Serial.println("Total slot tersedia: " + String(slotTersedia) + "/" + String(maxKapasitasParkir));
}

// ===============================
// FUNGSI INISIALISASI
// ===============================

void initializeUltrasonicSensors() {
  for (int i = 0; i < slotCount; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
  }
  Serial.println("Ultrasonic sensors initialized");
}

void inisialisasiSlotParkir() {
  Serial.println("\n=== Inisialisasi Status Slot Parkir ===");
  int slotTerisi = 0;
  
  for (int i = 0; i < slotCount; i++) {
    int jarak = getJarakUltrasonik(i);
    if (jarak <= 5) {
      statusSlotFisik[i] = false; // Terisi
      slotTerisi++;
      Serial.println("Slot " + String(slotNames[i]) + ": TERISI (Jarak: " + String(jarak) + " cm)");
    } else {
      statusSlotFisik[i] = true; // Kosong
      Serial.println("Slot " + String(slotNames[i]) + ": KOSONG (Jarak: " + String(jarak) + " cm)");
    }
  }
  
  slotTersedia = maxKapasitasParkir - slotTerisi;
  Serial.println("Slot tersedia saat startup: " + String(slotTersedia) + "/" + String(maxKapasitasParkir));
}