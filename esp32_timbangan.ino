#include <WiFi.h>
#include <WiFiClientSecure.h> // Wajib untuk koneksi HTTPS ke domain publik
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include <esp_now.h>
#include "DFRobotDFPlayerMini.h"
#if defined(ESP8266) || defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

// ---------------- LCD I2C 20x4 ----------------
const int LCD_ADDR = 0x27;
const int LCD_SDA = 21;
const int LCD_SCL = 22;
LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);

// ---------------- WIFI & SERVER ----------------
const char* WIFI_SSID     = "jamiroquai";
const char* WIFI_PASSWORD = "22222222";

const char* SERVER_URL = "https://seanta.my.id/api_sensor.php";
const char* API_KEY    = "Tmbgn-Pertamina-XYZ998";

// ---------------- DFPLAYER MINI (UART 2) ----------------
HardwareSerial mySerial2(2);
DFRobotDFPlayerMini myDFPlayer;

// Pin disesuaikan: GPIO 26 = ESP32 RX (ke TX DFPlayer), GPIO 27 = ESP32 TX (ke RX DFPlayer)
const int DFPLAYER_RX = 26;  // Hubungkan ke TX DFPlayer
const int DFPLAYER_TX = 27;  // Hubungkan ke RX DFPlayer (Gunakan Resistor 1K / 2K seri)

// ---------------- DATA BERAT (DARI ESP-NOW) ----------------
typedef struct struct_message {
  float berat;
} struct_message;

struct_message receivedData;

float beratMentahReceiver = 0;
float virtualTareOffset = 0;
float berat = 0;
float beratHalus = 0;

const float BERAT_DEADBAND = 0.30;
const float ALPHA_BERAT_NORMAL = 1.0;
const float ALPHA_BERAT_UKUR   = 1.0;

// Callback ESP-NOW (ESP32 Core v3.x)
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  beratMentahReceiver = receivedData.berat;
}

// ---------------- ULTRASONIK JSN-SR04T (TINGGI) ----------------
const int TRIG_PIN = 18;
const int ECHO_PIN = 19;

const unsigned long TIMEOUT_ECHO = 30000UL;
float tinggi_cm = 0;
float jarakSensor = 0;
float tinggiSebelumnya = 0;
bool tinggiSudahValid = false;

const float TINGGI_SENSOR_DARI_LANTAI = 196;

// ====== TEMPAT EDIT KALIBRASI TINGGI ======
// Ubah nilai di bawah ini kalau hasil tinggi masih meleset.
// Positif = menambah tinggi, negatif = mengurangi tinggi (satuan cm)
float TINGGI_OFFSET_KALIBRASI = 2.0;   // <-- koreksi +2 cm
// ==========================================

const float TINGGI_JARAK_MIN = 20;
const float TINGGI_JARAK_MAX = TINGGI_SENSOR_DARI_LANTAI + 10;
const float TINGGI_LONCATAN_MAX = 30;

int tinggiPenolakanBerturut = 0;
const int TINGGI_PENOLAKAN_MAX = 3;
unsigned long lastBacaTinggi = 0;
const unsigned long INTERVAL_BACA_TINGGI = 500;

// ---------------- OPTOCOUPLER (KELILING) - MODE INTERRUPT ----------------
#define SENSOR_PIN 35

// Dibagi 2 karena sekarang menghitung 2 tepi (naik + turun) per kedipan
float CM_PER_PULSA = 0.5;

volatile unsigned long pulseCount = 0;
volatile unsigned long lastPulseMicros = 0;
volatile bool pengukuranAktif = false;

// Debounce dalam MIKRODETIK
// 800us = sangat responsif, masih tahan noise
// Kalau muncul pulsa hantu  -> naikkan ke 1500 - 2000
// Kalau ada pulsa yg kelewat -> turunkan ke 200 - 300
const unsigned long DEBOUNCE_US = 800;

float keliling_cm = 0;
unsigned long lastPrint = 0;
const unsigned long INTERVAL_PRINT_NORMAL = 150;
const unsigned long INTERVAL_PRINT_UKUR   = 100;

// ---------------- TOMBOL FISIK ----------------
const int BUTTON_S_PIN = 32;
const int BUTTON_K_PIN = 33;
const unsigned long DEBOUNCE_TOMBOL_MS = 250;
int stateTombolS = HIGH;
int stateTombolK = HIGH;
unsigned long waktuTombolSTerakhir = 0;
unsigned long waktuTombolKTerakhir = 0;
bool voice1SudahDiputar = false;

// Prototype
void resetKeliling();

//====================================================
// ISR OPTOCOUPLER - dipanggil tiap tepi sinyal
//====================================================
void IRAM_ATTR isrPulsa() {
  unsigned long nowUs = micros();
  if (nowUs - lastPulseMicros < DEBOUNCE_US) return;  // tolak noise
  lastPulseMicros = nowUs;

  if (pengukuranAktif) pulseCount++;
}

//====================================================
// WIFI
//====================================================
void hubungkanWiFi() {
  Serial.print("Menghubungkan ke WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi terhubung, IP: ");
  Serial.println(WiFi.localIP());
}

//====================================================
// TAMPILKAN DI LCD
//====================================================
void tulisBarisLCD(int baris, String isi) {
  while (isi.length() < 20) {
    isi += ' ';
  }
  lcd.setCursor(0, baris);
  lcd.print(isi.substring(0, 20));
}

void tampilkanLCD() {
  tulisBarisLCD(0, "Berat : " + String(beratHalus, 2) + " kg");
  tulisBarisLCD(1, "Tinggi: " + String(tinggi_cm, 1) + " cm");

  if (pengukuranAktif) {
    tulisBarisLCD(2, "Perut : " + String(keliling_cm, 1) + " cm");
  } else {
    tulisBarisLCD(2, "Perut : belum ukur");
  }

  tulisBarisLCD(3, "WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Terhubung" : "Terputus"));
}

//====================================================
// KIRIM DATA KE SERVER (HTTPS)
//====================================================
void kirimDataKeServer() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi terputus, data tidak dikirim.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Bypass validasi sertifikat SSL/HTTPS
  client.setTimeout(5000); // Socket timeout 5 detik

  HTTPClient http;
  http.setTimeout(5000); // Timeout HTTPClient 5 detik agar ESP32 tidak freeze

  if (!http.begin(client, SERVER_URL)) {
    Serial.println("Gagal menginisialisasi koneksi HTTPClient.");
    client.stop();
    return;
  }
  http.addHeader("Content-Type", "application/json");

  // ================== DATA SENSOR ASLI (BUKAN DUMMY) ==================
  Serial.println("---------------------------------");
  Serial.println("MENGIRIM DATA SENSOR ASLI KE SERVER...");
  Serial.printf("Payload: Berat=%.2f, Tinggi=%.1f, Perut=%.1f\n", beratHalus, tinggi_cm, keliling_cm);
  Serial.println("---------------------------------");

  String payload = "{";
  payload += "\"api_key\":\"" + String(API_KEY) + "\",";
  payload += "\"berat_badan\":" + String(beratHalus, 2) + ",";
  payload += "\"tinggi_badan\":" + String(tinggi_cm, 1) + ",";
  payload += "\"lingkar_perut\":" + String(keliling_cm, 1);
  payload += "}";
  // ======================================================================

  int kodeRespon = http.POST(payload);

  if (kodeRespon > 0) {
    Serial.print("Kirim data OK, respon web: ");
    Serial.println(http.getString());
  } else {
    Serial.print("Kirim data GAGAL, kode error HTTPS: ");
    Serial.println(kodeRespon);
  }

  http.end();
  client.stop(); // Bebaskan RAM/SSL heap agar ESP32 tidak hang
}

//====================================================
// RESET KELILING
//====================================================
void resetKeliling() {
  noInterrupts();
  pulseCount = 0;
  lastPulseMicros = micros();
  interrupts();
  keliling_cm = 0;
  Serial.println(">>> KELILING DIRESET <<<");
}

//====================================================
// AKSI BERSAMA
//====================================================
void aksiMulaiUkurPerut() {
  resetKeliling();
  pengukuranAktif = true;
  lastPrint = 0;
  lastBacaTinggi = 0;
  Serial.println(">>> MULAI UKUR LINGKAR PERUT <<<");

  myDFPlayer.playMp3Folder(2); // Memutar file /mp3/0002.mp3
}

void aksiKirimData() {
  Serial.println(">>> KIRIM DATA KE WEB <<<");

  // Membaca ulang tinggi secara instan tepat sebelum dikirim agar nilainya paling segar
  jarakSensor = bacaTinggi();
  tinggi_cm = TINGGI_SENSOR_DARI_LANTAI - jarakSensor + TINGGI_OFFSET_KALIBRASI;
  if (tinggi_cm < 0) tinggi_cm = 0;

  kirimDataKeServer(); // Kirim nilai AKTUAL saat ini (mis. 60kg) ke website

  myDFPlayer.playMp3Folder(4); // Memutar file /mp3/0004.mp3

  // ================== RESET SESI UKUR LINGKAR PERUT SAJA ==================
  // CATATAN: berat/beratHalus/virtualTareOffset SENGAJA TIDAK disentuh di sini.
  // Berat harus tetap mengikuti angka aktual selama orangnya masih berdiri di
  // atas timbangan. Berat hanya akan kembali ke 0 secara alami saat beban
  // benar-benar diangkat dari timbangan (turun), bukan dipaksa nol saat kirim.
  tinggiSudahValid = false;                  // supaya pembacaan tinggi anak berikutnya mulai fresh
  tinggiSebelumnya = 0;
  tinggi_cm = 0;

  pengukuranAktif = false;
  resetKeliling();                           // reset lingkar perut ke 0 untuk sesi berikutnya

  lastPrint = 0;
  lastBacaTinggi = 0;

  Serial.println(">>> Sesi tinggi & lingkar perut direset, BERAT TETAP mengikuti sensor <<<");
  // ==========================================================================
}

//====================================================
// BACA TINGGI
//====================================================
float bacaTinggi() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long durasi = pulseIn(ECHO_PIN, HIGH, TIMEOUT_ECHO);
  if (durasi == 0) return tinggiSebelumnya;

  float d = durasi * 0.0343 / 2.0;
  if (d < TINGGI_JARAK_MIN || d > TINGGI_JARAK_MAX) return tinggiSebelumnya;

  if (!tinggiSudahValid) {
    tinggiSebelumnya = d;
    tinggiSudahValid = true;
    return d;
  }

  if (fabs(d - tinggiSebelumnya) > TINGGI_LONCATAN_MAX) {
    tinggiPenolakanBerturut++;
    if (tinggiPenolakanBerturut >= TINGGI_PENOLAKAN_MAX) {
      tinggiSebelumnya = d;
      tinggiPenolakanBerturut = 0;
      return d;
    }
    return tinggiSebelumnya;
  }

  tinggiPenolakanBerturut = 0;
  d = (0.7 * tinggiSebelumnya) + (0.3 * d);
  tinggiSebelumnya = d;
  return d;
}

//====================================================
// SETUP
//====================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting Main ESP32...");

  // Inisialisasi UART2 ke GPIO 26 (RX) & GPIO 27 (TX)
  mySerial2.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);

  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Menghubungkan WiFi..");

  hubungkanWiFi();

  // ================== INFO UNTUK SETUP ESP-NOW DI ESP32 BAWAH ==================
  // Salin MAC Address dan Channel di bawah ini ke kode espbawah.ino
  Serial.println("==================================");
  Serial.print("MAC Address ESP32 Utama : ");
  Serial.println(WiFi.macAddress());
  Serial.print("WiFi Channel saat ini   : ");
  Serial.println(WiFi.channel());
  Serial.println("==================================");
  // ===============================================================================

  // Inisialisasi DFPlayer Mini
  if (!myDFPlayer.begin(mySerial2, true, true)) {
    Serial.println(F("Gagal membaca DFPlayer. Cek koneksi SD Card/Kabel!"));
  } else {
    Serial.println(F("DFPlayer Mini Online."));
    delay(1000);             // Jeda 1 detik agar reset DFPlayer selesai
    myDFPlayer.volume(25);   // Atur volume (skala 0 - 30)
  }

  lcd.clear();

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error Inisialisasi ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("Menunggu data Loadcell via ESP-NOW...");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  // ---- Optocoupler mode interrupt ----
  pinMode(SENSOR_PIN, INPUT_PULLUP);
  lastPulseMicros = micros();
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), isrPulsa, CHANGE);

  pinMode(BUTTON_S_PIN, INPUT_PULLUP);
  pinMode(BUTTON_K_PIN, INPUT_PULLUP);
  stateTombolS = digitalRead(BUTTON_S_PIN);
  stateTombolK = digitalRead(BUTTON_K_PIN);

  Serial.println("==================================");
  Serial.println("S = Mulai ukur lingkar perut");
  Serial.println("R = Reset & Stop ukur lingkar");
  Serial.println("T = Tare Load Cell (Virtual)");
  Serial.println("K = Kirim data ke web sekarang");
  Serial.println("==================================");
  Serial.print("Offset kalibrasi tinggi: ");
  Serial.print(TINGGI_OFFSET_KALIBRASI, 1);
  Serial.println(" cm");
}

//====================================================
// LOOP
//====================================================
void loop() {
  unsigned long now = millis();

  // ================== LOGIKA BERAT (ESP-NOW) ==================
  berat = beratMentahReceiver - virtualTareOffset;

  if (fabs(berat) < BERAT_DEADBAND) {
    berat = 0;
  }

  float alphaBerat = pengukuranAktif ? ALPHA_BERAT_UKUR : ALPHA_BERAT_NORMAL;
  beratHalus = (alphaBerat * berat) + ((1.0 - alphaBerat) * beratHalus);

  // ================== TINGGI ==================
  if (now - lastBacaTinggi >= INTERVAL_BACA_TINGGI) {
    lastBacaTinggi = now;
    jarakSensor = bacaTinggi();
    tinggi_cm = TINGGI_SENSOR_DARI_LANTAI - jarakSensor + TINGGI_OFFSET_KALIBRASI;
    if (tinggi_cm < 0) tinggi_cm = 0;
  }

  // ================== KELILING ==================
  noInterrupts();
  unsigned long pulsaSekarang = pulseCount;
  interrupts();
  keliling_cm = pulsaSekarang * CM_PER_PULSA;

  // ==================================================
  // SERIAL COMMAND & TOMBOL
  // ==================================================
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 'S' || cmd == 's') aksiMulaiUkurPerut();
    else if (cmd == 'R' || cmd == 'r') {
      pengukuranAktif = false;
      resetKeliling();
      lastPrint = 0;
      lastBacaTinggi = 0;
      Serial.println(">>> STOP UKUR LINGKAR PERUT <<<");
      myDFPlayer.playMp3Folder(3); // Memutar file /mp3/0003.mp3
    }
    else if (cmd == 'T' || cmd == 't') {
      virtualTareOffset = beratMentahReceiver;
      Serial.println("Tare load cell (Virtual) selesai.");
    }
    else if (cmd == 'K' || cmd == 'k') aksiKirimData();
  }

  int bacaTombolS = digitalRead(BUTTON_S_PIN);
  if (bacaTombolS != stateTombolS) {
    stateTombolS = bacaTombolS;
    if (stateTombolS == LOW && (now - waktuTombolSTerakhir > DEBOUNCE_TOMBOL_MS)) {
      waktuTombolSTerakhir = now;
      aksiMulaiUkurPerut();
    }
  }

  int bacaTombolK = digitalRead(BUTTON_K_PIN);
  if (bacaTombolK != stateTombolK) {
    stateTombolK = bacaTombolK;
    if (stateTombolK == LOW && (now - waktuTombolKTerakhir > DEBOUNCE_TOMBOL_MS)) {
      waktuTombolKTerakhir = now;
      aksiKirimData();
    }
  }

  // ==================================================
  // TAMPILKAN DATA
  // ==================================================
  unsigned long intervalPrintSaatIni = pengukuranAktif ? INTERVAL_PRINT_UKUR : INTERVAL_PRINT_NORMAL;
  if (now - lastPrint >= intervalPrintSaatIni) {
    lastPrint = now;

    Serial.print("Berat: "); Serial.print(beratHalus, 2); Serial.print(" kg");
    Serial.print(" | Tinggi: "); Serial.print(tinggi_cm, 1); Serial.print(" cm");

    if (pengukuranAktif) {
      Serial.print(" | Pulsa: "); Serial.print(pulsaSekarang);
      Serial.print(" | Keliling: "); Serial.print(keliling_cm, 1); Serial.print(" cm");
    }
    Serial.println();

    tampilkanLCD();

    // Memutar Suara 1 (/mp3/0001.mp3) sekali saat layar pertama kali aktif
    if (!voice1SudahDiputar) {
      voice1SudahDiputar = true;
      myDFPlayer.playMp3Folder(1);
    }
  }
}
