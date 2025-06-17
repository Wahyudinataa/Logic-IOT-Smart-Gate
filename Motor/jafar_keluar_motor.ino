#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// servo gate keluar motor
Servo gateKeluar;
#define SERVO_KELUAR 12

// Infrared sensor untuk keluar
#define IR_KELUAR_PIN 33

// SET UP
void setup() {
  Serial.begin(115200);
  
  // setup lcd
  lcd.init();
  lcd.backlight();
  
  // setup servo untuk gate keluar
  gateKeluar.attach(SERVO_KELUAR);
  gateKeluar.write(0);
  delay(1000); // Beri waktu servo untuk bergerak ke posisi awal
  
  // Infrared sensor setup
  pinMode(IR_KELUAR_PIN, INPUT);
  
  Serial.println("=== Sistem Keluar Motor Siap ===");
  Serial.println("Gate Keluar dalam posisi tertutup (0 derajat)");
  
  delay(2000);
}

void loop() {
  // Cek sensor infrared untuk keluar
  keluarmotor();
  
  delay(100); // Delay kecil untuk stabilitas
}

void keluarmotor() {
  static bool kendaraanTerdeteksiSebelumnya = false;
  static unsigned long waktuDeteksi = 0;
  bool kendaraanTerdeteksi = !digitalRead(IR_KELUAR_PIN);
  
  // Deteksi transisi dari tidak ada ke ada kendaraan
  if (kendaraanTerdeteksi && !kendaraanTerdeteksiSebelumnya) {
    waktuDeteksi = millis();
    Serial.println("=== Kendaraan Keluar Terdeteksi ===");
    
    // Buka gate untuk keluar
    bukaGateKeluar();
  }
  
  kendaraanTerdeteksiSebelumnya = kendaraanTerdeteksi;
}

void bukaGateKeluar() {
  Serial.println("=== MOTOR TERDETEKSI ===");
  
  // Tampilkan pesan di LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Kendaraan Keluar");
  lcd.setCursor(0, 1);
  lcd.print("Gate Terbuka...");
  
  // Buka gate keluar secara bertahap
  Serial.println("Servo Keluar bergerak dari 0° ke 90°");
  for (int pos = 0; pos <= 90; pos += 10) {
    gateKeluar.write(pos);
    delay(30); // Gerak lebih cepat untuk keluar
  }
  
  lcd.setCursor(0, 1);
  lcd.print("Silakan Keluar  ");
  delay(5000);
  
  // Tampilkan pesan penutupan
  lcd.setCursor(0, 1);
  lcd.print("Gate Menutup... ");
  // Tutup gate keluar secara bertahap
  for (int pos = 90; pos >= 0; pos -= 10) {
    gateKeluar.write(pos);
    delay(30);
  }
  
  Serial.println("Gate Keluar tertutup (0°)");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Selamat Jalan");
  lcd.setCursor(0, 1);
  lcd.print("Gate Tertutup");
  
  delay(2000);
  
  // Kembali ke tampilan default
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Keluar");
  lcd.setCursor(0, 1);
  lcd.print("Siap Deteksi");
}