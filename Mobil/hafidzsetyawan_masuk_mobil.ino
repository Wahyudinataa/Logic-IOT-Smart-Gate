#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <ESP32Servo.h>

// Objek eksternal yang digunakan (asumsikan sudah dideklarasikan di file utama)
extern MFRC522 rfid;
extern MFRC522::MIFARE_Key key;
extern LiquidCrystal_I2C lcd;
extern Servo gateMasuk;

extern int slotTersedia;
extern int maxKapasitasParkir;
extern bool statusSlotFisik[];
extern const char slotNames[];

struct DataMahasiswa {
  String nama;
  String nim;
  String fakultas;
  String status;
};

// ===== Deklarasi fungsi pendukung yang diasumsikan sudah dibuat di tempat lain =====
int hitungSlotFisikKosong();
char cariSlotKosongPertama();
void tampilkanPesan(String baris1, String baris2);
void tampilkanArahanParkir(char slotKosong);
void tampilkanDataMahasiswa(DataMahasiswa data);
void printStudentData(DataMahasiswa data);
DataMahasiswa bacaDataMahasiswaDariKartu();

// ===============================
// LOGIKA MASUK MOBIL
// ===============================

void prosesMasukMobil() {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    Serial.println("\n=== MASUK MOBIL - KTM TERDETEKSI ===");
    
    // Baca data mahasiswa dari kartu RFID
    DataMahasiswa dataMahasiswa = bacaDataMahasiswaDariKartu();
    
    if (dataMahasiswa.nama != "") {
      printStudentData(dataMahasiswa);
      tampilkanDataMahasiswa(dataMahasiswa);
      
      if (validasiAksesMasuk(dataMahasiswa)) {
        eksekusiMasukMobil(dataMahasiswa);
      } else {
        tolakAksesMasuk(dataMahasiswa);
      }
    } else {
      Serial.println("✗ Gagal membaca data dari KTM");
      tampilkanPesan("Error Baca", "Data KTM");
      delay(3000);
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    delay(2000);
  }
}

bool validasiAksesMasuk(DataMahasiswa data) {
  bool statusAktif = (data.status.indexOf("AKTIF") >= 0);
  bool adaSlotTracking = (slotTersedia > 0);
  bool adaSlotFisik = (hitungSlotFisikKosong() > 0);
  
  Serial.println("VALIDASI MASUK:");
  Serial.println("- Status Aktif: " + String(statusAktif ? "YA" : "TIDAK"));
  Serial.println("- Slot Tracking: " + String(slotTersedia) + "/" + String(maxKapasitasParkir));
  Serial.println("- Slot Fisik Kosong: " + String(hitungSlotFisikKosong()));
  
  return (statusAktif && adaSlotTracking && adaSlotFisik);
}

void eksekusiMasukMobil(DataMahasiswa data) {
  char slotKosong = cariSlotKosongPertama();
  
  Serial.println("✓ AKSES MASUK DIIZINKAN");
  Serial.println("REKOMENDASI: Slot " + String(slotKosong));
  
  slotTersedia--;
  Serial.println("TRACKING: Slot berkurang menjadi: " + String(slotTersedia) + "/" + String(maxKapasitasParkir));
  
  tampilkanArahanParkir(slotKosong);
  bukaGateMasuk();
}

void tolakAksesMasuk(DataMahasiswa data) {
  Serial.println("✗ AKSES MASUK DITOLAK");
  
  if (data.status.indexOf("AKTIF") < 0) {
    Serial.println("Alasan: Status Tidak Aktif");
    tampilkanPesan("Akses Ditolak", "Status Non-Aktif");
  } else if (slotTersedia <= 0) {
    Serial.println("Alasan: Slot Penuh (Tracking)");
    tampilkanPesan("Area Penuh", "Slot:" + String(slotTersedia) + "/" + String(maxKapasitasParkir));
  } else if (hitungSlotFisikKosong() <= 0) {
    Serial.println("Alasan: Semua Tempat Terisi (Fisik)");
    tampilkanPesan("Tempat Penuh", "Semua Terisi");
  }
  
  delay(3000);
}

void bukaGateMasuk() {
  Serial.println("=== MEMBUKA GATE MASUK ===");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Selamat Datang!");
  lcd.setCursor(0, 1);
  lcd.print("Slot:" + String(slotTersedia) + "/" + String(maxKapasitasParkir) + " Gate Buka");
  
  for (int pos = 0; pos <= 90; pos += 5) {
    gateMasuk.write(pos);
    delay(50);
  }
  
  lcd.setCursor(0, 1);
  lcd.print("Silakan Masuk   ");
  delay(8000);
  
  lcd.setCursor(0, 1);
  lcd.print("Gate Menutup... ");
  
  for (int pos = 90; pos >= 0; pos -= 5) {
    gateMasuk.write(pos);
    delay(50);
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gate Tertutup");
  lcd.setCursor(0, 1);
  lcd.print("Akses Selesai");
  delay(2000);
  
  Serial.println("Gate Masuk tertutup - Proses selesai");
}
