#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// RFID RC522
#define SS_PIN 5
#define RST_PIN 4
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo gate masuk motor
Servo gateMasuk;
#define SERVO_MASUK 13

// Infrared sensor untuk deteksi kendaraan lewat
#define INFRARED_PIN 32
bool gateStatusTerbuka = false;
unsigned long waktuGateTerbuka = 0;
const unsigned long TIMEOUT_GATE = 30000; // 30 detik timeout maksimal

// Ultrasonic sensor pins
const int trigPins[] = {14, 27, 26, 25};
const int echoPins[] = {17, 16, 2, 15};
const int slotCount = 4;

// Struktur data mahasiswa yang dibaca dari kartu
struct DataMahasiswa {
  String nama;
  String nim;
  String fakultas;
  String status;
};

void setup() {
  Serial.begin(115200);
  
  // LCD setup
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Parkir");
  lcd.setCursor(0, 1);
  lcd.print("KTM RFID Ready");
  
  // RFID setup
  SPI.begin();
  rfid.PCD_Init();
  
  // Inisialisasi kunci RFID (default key)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  // Servo setup
  gateMasuk.attach(SERVO_MASUK);
  gateMasuk.write(0); // posisi tertutup
  delay(1000); // Beri waktu servo untuk bergerak ke posisi awal
  
  // Infrared sensor setup
  pinMode(INFRARED_PIN, INPUT);
  
  // Ultrasonic pins
  for (int i = 0; i < slotCount; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
  }
  
  Serial.println("=== Sistem Parkir KTM RFID Siap ===");
  Serial.println("Tempelkan KTM untuk membaca data...");
  Serial.println("Gate dalam posisi tertutup (0 derajat)");
  Serial.println("Infrared sensor siap pada pin GPIO 32");
  
  delay(2000);
}

void loop() {
  int slotKosong = hitungSlotKosong();
  tampilkanSlot(slotKosong);
  
  // Cek kondisi gate terbuka dan sensor infrared
  if (gateStatusTerbuka) {
    cekInfraredDanTutupGate();
  }
  
  // Proses pembacaan KTM (hanya jika gate tertutup)
  if (!gateStatusTerbuka && rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.println("\n=== KTM Terdeteksi ===");
    
    // Baca data mahasiswa dari kartu RFID
    DataMahasiswa dataMahasiswa = bacaDataMahasiswaDariKartu();
    
    if (dataMahasiswa.nama != "") {
      // Data berhasil dibaca
      Serial.println("=== Data Mahasiswa dari KTM ===");
      Serial.println("Nama: " + dataMahasiswa.nama);
      Serial.println("NIM: " + dataMahasiswa.nim);
      Serial.println("Fakultas: " + dataMahasiswa.fakultas);
      Serial.println("Status: " + dataMahasiswa.status);
      
      // Tampilkan di LCD
      tampilkanDataMahasiswa(dataMahasiswa);
      
      // Cek status dan slot kosong
      if (dataMahasiswa.status.indexOf("AKTIF") >= 0 && slotKosong > 0) {
        Serial.println("✓ Akses Diizinkan - Gate Akan Dibuka");
        bukaGate(); // Panggil fungsi buka gate
      } else if (dataMahasiswa.status.indexOf("AKTIF") < 0) {
        Serial.println("✗ Akses Ditolak - Status Tidak Aktif");
        tampilkanPesan("Akses Ditolak", "Status Non-Aktif");
        delay(3000);
      } else {
        Serial.println("✗ Akses Ditolak - Parkir Penuh");
        tampilkanPesan("Parkir Penuh", "Coba Lagi Nanti");
        delay(3000);
      }
    } else {
      Serial.println("✗ Gagal membaca data dari KTM");
      tampilkanPesan("Error Baca", "Data KTM");
      delay(3000);
    }
    
    // Halt kartu setelah selesai diproses
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    
    // Jeda sebelum siap membaca kartu berikutnya
    delay(2000);
  }
  
  delay(100); // Kurangi delay untuk responsif yang lebih baik
}

void cekInfraredDanTutupGate() {
  // Baca status infrared sensor (LOW = terdeteksi objek, HIGH = tidak ada objek)
  bool infraredStatus = digitalRead(INFRARED_PIN);
  
  // Deteksi perubahan dari HIGH ke LOW (objek melewati sensor)
  static bool infraredStatusSebelumnya = HIGH;
  static bool objektTerdeteksi = false;
  static unsigned long waktuDeteksi = 0;
  
  // Jika ada perubahan dari HIGH ke LOW
  if (infraredStatusSebelumnya == HIGH && infraredStatus == LOW) {
    objektTerdeteksi = true;
    waktuDeteksi = millis();
    Serial.println("Infrared: Objek memasuki area sensor");
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Kendaraan");
    lcd.setCursor(0, 1);
    lcd.print("Terdeteksi...");
  }
  
  // Jika objek telah melewati sensor (HIGH lagi) setelah terdeteksi
  if (objektTerdeteksi && infraredStatus == HIGH && (millis() - waktuDeteksi > 1000)) {
    Serial.println("Infrared: Objek telah melewati sensor - Menutup gate");
    objektTerdeteksi = false;
    tutupGate();
  }
  
  // Timeout safety - tutup gate setelah 30 detik jika tidak ada deteksi
  if (millis() - waktuGateTerbuka > TIMEOUT_GATE) {
    Serial.println("Timeout: Gate ditutup secara otomatis setelah 30 detik");
    tutupGate();
  }
  
  infraredStatusSebelumnya = infraredStatus;
}

DataMahasiswa bacaDataMahasiswaDariKartu() {
  DataMahasiswa data;
  
  Serial.println("Membaca data dari sektor kartu RFID...");
  
  // Baca dari beberapa blok yang mungkin berisi data
  // Biasanya data disimpan di sektor 1, 2, atau 4
  byte sektor[] = {1, 2, 4, 8}; // Sektor yang akan dicoba
  int jumlahSektor = sizeof(sektor) / sizeof(sektor[0]);
  
  String allData = "";
  
  for (int s = 0; s < jumlahSektor; s++) {
    for (byte blok = sektor[s] * 4; blok < (sektor[s] * 4) + 3; blok++) {
      // Skip blok trailer (sektor trailer)
      if ((blok + 1) % 4 == 0) continue;
      
      String blockData = bacaBlokData(blok);
      if (blockData != "") {
        allData += blockData;
        Serial.println("Blok " + String(blok) + ": " + blockData);
      }
    }
  }
  
  // Parse data yang terbaca
  if (allData.length() > 0) {
    data = parseDataMahasiswa(allData);
  }
  
  return data;
}

String bacaBlokData(byte blok) {
  byte buffer[18];
  byte size = sizeof(buffer);
  String result = "";
  
  // Otentikasi dengan kunci A
  MFRC522::StatusCode status = rfid.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A, blok, &key, &(rfid.uid)
  );
  
  if (status == MFRC522::STATUS_OK) {
    // Baca data dari blok
    status = rfid.MIFARE_Read(blok, buffer, &size);
    
    if (status == MFRC522::STATUS_OK) {
      // Konversi byte ke string
      for (byte i = 0; i < 16; i++) {
        if (buffer[i] >= 32 && buffer[i] <= 126) { // Karakter ASCII yang bisa dibaca
          result += (char)buffer[i];
        } else if (buffer[i] == 0) {
          break; // Berhenti jika menemukan null terminator
        }
      }
      
      // Bersihkan karakter tidak perlu di akhir
      result.trim();
    }
  }
  
  return result;
}

DataMahasiswa parseDataMahasiswa(String rawData) {
  DataMahasiswa data;
  
  Serial.println("Raw data: " + rawData);
  
  // Metode parsing berdasarkan delimiter umum
  // Coba beberapa format umum yang mungkin digunakan
  
  // Format 1: NAMA|NIM|FAKULTAS|STATUS
  if (rawData.indexOf('|') > 0) {
    int pos1 = rawData.indexOf('|');
    int pos2 = rawData.indexOf('|', pos1 + 1);
    int pos3 = rawData.indexOf('|', pos2 + 1);
    
    if (pos1 > 0 && pos2 > pos1 && pos3 > pos2) {
      data.nama = rawData.substring(0, pos1);
      data.nim = rawData.substring(pos1 + 1, pos2);
      data.fakultas = rawData.substring(pos2 + 1, pos3);
      data.status = rawData.substring(pos3 + 1);
    }
  }
  // Format 2: NAMA,NIM,FAKULTAS,STATUS
  else if (rawData.indexOf(',') > 0) {
    int pos1 = rawData.indexOf(',');
    int pos2 = rawData.indexOf(',', pos1 + 1);
    int pos3 = rawData.indexOf(',', pos2 + 1);
    
    if (pos1 > 0 && pos2 > pos1 && pos3 > pos2) {
      data.nama = rawData.substring(0, pos1);
      data.nim = rawData.substring(pos1 + 1, pos2);
      data.fakultas = rawData.substring(pos2 + 1, pos3);
      data.status = rawData.substring(pos3 + 1);
    }
  }
  // Format 3: NAMA;NIM;FAKULTAS;STATUS
  else if (rawData.indexOf(';') > 0) {
    int pos1 = rawData.indexOf(';');
    int pos2 = rawData.indexOf(';', pos1 + 1);
    int pos3 = rawData.indexOf(';', pos2 + 1);
    
    if (pos1 > 0 && pos2 > pos1 && pos3 > pos2) {
      data.nama = rawData.substring(0, pos1);
      data.nim = rawData.substring(pos1 + 1, pos2);
      data.fakultas = rawData.substring(pos2 + 1, pos3);
      data.status = rawData.substring(pos3 + 1);
    }
  }
  // Format 4: Jika tidak ada delimiter, coba parsing berdasarkan panjang tetap
  else if (rawData.length() >= 20) {
    // Asumsikan format: 16 char nama + 10 char nim + sisanya fakultas dan status
    data.nama = rawData.substring(0, 16);
    data.nim = rawData.substring(16, 26);
    
    String sisaData = rawData.substring(26);
    if (sisaData.length() > 10) {
      data.fakultas = sisaData.substring(0, 10);
      data.status = sisaData.substring(10);
    } else {
      data.fakultas = sisaData;
      data.status = "AKTIF";
    }
  }
  // Format 5: Semua data dalam satu string, ambil yang bisa diidentifikasi
  else {
    // Cari pola NIM (angka 8-10 digit)
    for (int i = 0; i < rawData.length() - 7; i++) {
      String sub = rawData.substring(i, i + 10);
      bool isNIM = true;
      for (int j = 0; j < sub.length(); j++) {
        if (!isDigit(sub.charAt(j))) {
          isNIM = false;
          break;
        }
      }
      if (isNIM && sub.length() >= 8) {
        data.nim = sub.substring(0, 10);
        data.nama = rawData.substring(0, i);
        data.nama.trim();
        data.fakultas = rawData.substring(i + 10);
        data.fakultas.trim();
        data.status = "AKTIF";
        break;
      }
    }
    
    // Jika tidak ada pola NIM yang jelas, ambil sebagai nama
    if (data.nama == "") {
      data.nama = rawData.substring(0, min(16, (int)rawData.length()));
      data.nama.trim();
      data.nim = "Unknown";
      data.fakultas = "Unknown";
      data.status = "AKTIF";
    }
  }
  
  // Bersihkan data
  data.nama.trim();
  data.nim.trim();
  data.fakultas.trim();
  data.status.trim();
  
  // Set default jika kosong
  if (data.status == "") data.status = "AKTIF";
  
  return data;
}

void tampilkanDataMahasiswa(DataMahasiswa data) {
  // Tampilkan bergantian antara nama-nim dan fakultas-status
  for (int i = 0; i < 2; i++) { // Kurangi iterasi untuk mempercepat
    // Tampilkan nama dan NIM
    lcd.clear();
    lcd.setCursor(0, 0);
    String nama = data.nama;
    if (nama.length() > 16) nama = nama.substring(0, 16);
    lcd.print(nama);
    
    lcd.setCursor(0, 1);
    String nim = "NIM:" + data.nim;
    if (nim.length() > 16) nim = nim.substring(0, 16);
    lcd.print(nim);
    
    delay(1500);
    
    // Tampilkan fakultas dan status
    lcd.clear();
    lcd.setCursor(0, 0);
    String fakultas = data.fakultas;
    if (fakultas.length() > 16) fakultas = fakultas.substring(0, 16);
    lcd.print(fakultas);
    
    lcd.setCursor(0, 1);
    String status = "Status:" + data.status;
    if (status.length() > 16) status = status.substring(0, 16);
    lcd.print(status);
    
    delay(1500);
  }
}

void tampilkanPesan(String baris1, String baris2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(baris1);
  lcd.setCursor(0, 1);
  lcd.print(baris2);
}

void bukaGate() {
  Serial.println("=== MEMBUKA GATE ===");
  
  // Tampilkan pesan di LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Selamat Datang!");
  lcd.setCursor(0, 1);
  lcd.print("Gate Terbuka...");
  
  // Buka gate secara bertahap untuk efek yang lebih halus
  Serial.println("Servo bergerak dari 0° ke 90°");
  for (int pos = 0; pos <= 90; pos += 5) {
    gateMasuk.write(pos);
    delay(50); // Gerak halus
  }
  
  Serial.println("Gate terbuka penuh (90°)");
  
  // Set status gate terbuka
  gateStatusTerbuka = true;
  waktuGateTerbuka = millis();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Silakan Masuk");
  lcd.setCursor(0, 1);
  lcd.print("Menunggu Lewat..");
  
  Serial.println("Gate akan tertutup setelah kendaraan melewati sensor infrared");
}

void tutupGate() {
  if (!gateStatusTerbuka) return; // Jika sudah tertutup, tidak perlu ditutup lagi
  
  Serial.println("=== MENUTUP GATE ===");
  
  // Tampilkan pesan penutupan
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Terima Kasih");
  lcd.setCursor(0, 1);
  lcd.print("Gate Menutup...");
  
  // Tutup gate secara bertahap
  Serial.println("Servo bergerak dari 90° ke 0°");
  for (int pos = 90; pos >= 0; pos -= 5) {
    gateMasuk.write(pos);
    delay(50); // Gerak halus
  }
  
  Serial.println("Gate tertutup (0°)");
  
  // Reset status gate
  gateStatusTerbuka = false;
  waktuGateTerbuka = 0;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gate Tertutup");
  lcd.setCursor(0, 1);
  lcd.print("Siap Scan KTM");
  
  delay(2000);
}

int hitungSlotKosong() {
  int kosong = 0;
  for (int i = 0; i < slotCount; i++) {
    digitalWrite(trigPins[i], LOW);
    delayMicroseconds(2);
    digitalWrite(trigPins[i], HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPins[i], LOW);
    
    long durasi = pulseIn(echoPins[i], HIGH, 30000);
    if (durasi == 0) durasi = 30000;
    int jarak = durasi * 0.034 / 2;
    
    if (jarak > 15) kosong++;
  }
  return kosong;
}

void tampilkanSlot(int jumlah) {
  static unsigned long lastUpdate = 0;
  
  if (millis() - lastUpdate > 3000) { // Update setiap 3 detik
    if (!rfid.PICC_IsNewCardPresent() && !gateStatusTerbuka) { // Hanya tampilkan jika tidak ada kartu dan gate tertutup
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Slot Kosong: ");
      lcd.print(jumlah);
      lcd.setCursor(0, 1);
      lcd.print("Tempelkan KTM");
    }
    lastUpdate = millis();
  }
}