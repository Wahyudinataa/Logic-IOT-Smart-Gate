#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// pin sensor ultrasonik
const int trigPins[] = {14, 27, 26, 25};
const int echoPins[] = {17, 16, 2, 15};
const int slotCount = 4;
const char slotNames[] = {'A', 'B', 'C', 'D'};

// SISTEM TRACKING PARKIR
int slotTersedia = 4;
int maxKapasitasParkir = 4;
bool statusSlotFisik[4];

// Variable
unsigned long lastSlotCheck = 0;
unsigned long lastDisplayUpdate = 0;

void setup() {
  Serial.begin(115200);
  
  initializeLCD();
  initializeUltrasonicSensors();
  inisialisasiSlotParkir();
  
  printSystemReady();
  delay(2000);
}

void loop() {
  // Update status fisik slot dan ketersediaan parkir
  updateKetersediaanParkir();
  
  // Tampilkan informasi ketersediaan di LCD
  tampilkanInfoKetersediaan();
  
  delay(100); // Delay kecil untuk stabilitas
}

// ===============================
// LOGIKA KETERSEDIAAN PARKIR
// ===============================

void updateKetersediaan() {
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
    
    // Update jumlah slot tersedia berdasarkan status fisik
    slotTersedia = hitungSlotFisikKosong();
    
    lastSlotCheck = millis();
    
    // Debug info ketersediaan
    Serial.println("KETERSEDIAAN - Slot Kosong: " + String(slotTersedia) + "/" + String(maxKapasitasParkir));
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
// TAMPILAN LCD KETERSEDIAAN
// ===============================

void tampilkanInfoKetersediaan() {
  // Update display setiap 3 detik
  if (millis() - lastDisplayUpdate > 3000) {
    lcd.clear();
    
    // Baris pertama: Jumlah slot tersedia
    lcd.setCursor(0, 0);
    lcd.print("Slot Tersedia:");
    lcd.print(slotTersedia);
    lcd.print("/");
    lcd.print(maxKapasitasParkir);
    
    // Baris kedua: Status ketersediaan
    lcd.setCursor(0, 1);
    if (cekKetersediaanParkir()) {
      lcd.print("Parkir Tersedia");
    } else {
      lcd.print("Area Penuh!");
    }
    
    lastDisplayUpdate = millis();
  }
}

// ===============================
// FUNGSI PENDUKUNG
// ===============================

int getJarakUltrasonik(int slot) {
  digitalWrite(trigPins[slot], LOW);
  delayMicroseconds(2);
  digitalWrite(trigPins[slot], HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPins[slot], LOW);
  
  long durasi = pulseIn(echoPins[slot], HIGH, 30000);
  if (durasi == 0) durasi = 30000;
  return durasi * 0.034 / 2;
}

// ===============================
// FUNGSI INISIALISASI
// ===============================

void initializeLCD() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Parkir");
  lcd.setCursor(0, 1);
  lcd.print("Ketersediaan");
  delay(2000);
}

void initializeUltrasonicSensors() {
  for (int i = 0; i < slotCount; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
  }
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

void printSystemReady() {
  Serial.println("=== Sistem Ketersediaan Parkir ===");
  Serial.println("Monitoring ketersediaan slot parkir...");
  Serial.println("Slot tersedia: " + String(slotTersedia) + "/" + String(maxKapasitasParkir));
}