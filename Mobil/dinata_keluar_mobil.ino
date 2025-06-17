#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo gate keluar mobil
Servo gateKeluar;
#define SERVO_KELUAR 12

// Infrared sensor untuk keluar
#define IR_KELUAR_PIN 33

// Tracking slot parkir (diasumsikan di-update dari sistem utama)
extern int slotTersedia;
extern int maxKapasitasParkir;

// ===============================
// SETUP
// ===============================
void setup() {
  Serial.begin(115200);

  pinMode(IR_KELUAR_PIN, INPUT);
  gateKeluar.attach(SERVO_KELUAR);

  lcd.init();
  lcd.backlight();

  Serial.println("LOGIKA KELUAR MOBIL AKTIF");
}

// ===============================
// LOOP UTAMA
// ===============================
void loop() {
  prosesKeluarMobil();
  delay(100);
}

// ===============================
// LOGIKA KELUAR MOBIL
// ===============================
void prosesKeluarMobil() {
  static bool kendaraanTerdeteksiSebelumnya = false;

  bool kendaraanTerdeteksi = !digitalRead(IR_KELUAR_PIN);

  if (kendaraanTerdeteksi && !kendaraanTerdeteksiSebelumnya) {
    eksekusiKeluarMobil();
  }

  kendaraanTerdeteksiSebelumnya = kendaraanTerdeteksi;
}

void eksekusiKeluarMobil() {
  Serial.println("=== KELUAR MOBIL TERDETEKSI ===");

  if (slotTersedia < maxKapasitasParkir) {
    slotTersedia++;
    Serial.println("TRACKING: Slot bertambah menjadi: " + String(slotTersedia) + "/" + String(maxKapasitasParkir));
  } else {
    Serial.println("WARNING: Deteksi keluar tapi slot sudah maksimal!");
  }

  bukaGateKeluar();
}

void bukaGateKeluar() {
  Serial.println("=== MEMBUKA GATE KELUAR ===");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Kendaraan Keluar");
  lcd.setCursor(0, 1);
  lcd.print("Slot:" + String(slotTersedia) + "/" + String(maxKapasitasParkir));

  for (int pos = 0; pos <= 90; pos += 10) {
    gateKeluar.write(pos);
    delay(30);
  }

  lcd.setCursor(0, 1);
  lcd.print("Silakan Keluar  ");
  delay(5000);

  lcd.setCursor(0, 1);
  lcd.print("Gate Menutup... ");
  for (int pos = 90; pos >= 0; pos -= 10) {
    gateKeluar.write(pos);
    delay(30);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Selamat Jalan");
  lcd.setCursor(0, 1);
  lcd.print("Slot:" + String(slotTersedia));
  delay(2000);

  Serial.println("Gate Keluar tertutup - Proses selesai");
}