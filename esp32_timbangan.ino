#include <HX711_ADC.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>
#if defined(ESP8266) || defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

// ---------------- LCD I2C 20x4 ----------------
// Alamat umum 0x27 atau 0x3F, cek dengan I2C scanner kalau layar tidak tampil.
const int LCD_ADDR = 0x27;
const int LCD_SDA = 21;
const int LCD_SCL = 22;

LiquidCrystal_I2C lcd(LCD_ADDR, 20, 4);

// ---------------- WIFI & SERVER ----------------
const char* WIFI_SSID     = "PERTAMINA123";
const char* WIFI_PASSWORD = "Bismillah";

// Ganti sesuai IP lokal komputer XAMPP (cek dengan ipconfig)
const char* SERVER_URL = "http://192.168.1.196/Timbangan/api_sensor.php";
const char* API_KEY    = "ubah-kunci-ini";

// Kirim ke web TIDAK otomatis lagi. Tekan 'K' di Serial Monitor untuk kirim.

// ---------------- LOAD CELL (BERAT) ----------------
const int HX711_dout = 4;
const int HX711_sck  = 15;

HX711_ADC LoadCell(HX711_dout, HX711_sck);

// Diambil dari kode Uno yang sudah dikalibrasi & terbukti akurat di instalasi
// load cell yang sama (fisiknya tetap ESP32, cuma angka kalibrasinya dipakai).
float calibrationValue = -22982.2871;

float berat = 0;         // nilai mentah dari load cell (deadband diterapkan)
float beratHalus = 0;    // nilai yang ditampilkan (dihaluskan saat ukur perut)

const float OFFSET_KALIBRASI = 0.75; // sama seperti kode Uno: berat = LoadCell.getData() - 0.75

// Untuk load cell 200kg, noise per satuan kg biasanya lebih besar
// dibanding load cell kecil, jadi deadband dinaikkan agar angka "0.00"
// tidak berkedip-kedip saat timbangan benar-benar kosong.
const float BERAT_DEADBAND = 0.30;  // kg, di bawah ini dianggap 0 (tidak ada beban)

// Berat selalu real-time
const float ALPHA_BERAT_NORMAL = 1.0;
const float ALPHA_BERAT_UKUR   = 1.0;

// ---------------- ULTRASONIK JSN-SR04T (TINGGI) ----------------
const int TRIG_PIN = 18;
const int ECHO_PIN = 19;   // Ganti sesuai wiring

const unsigned long TIMEOUT_ECHO = 30000UL;

float tinggi_cm = 0;         // hasil AKHIR = tinggi badan (sudah dikonversi dari jarak sensor)
float jarakSensor = 0;       // jarak mentah sensor->objek TERAKHIR (buat debug di Serial)
float tinggiSebelumnya = 0;  // jarak mentah sensor->kepala (dipakai internal untuk filter)
bool tinggiSudahValid = false;

// Sensor dipasang di plafon/atas menghadap ke bawah. Tinggi badan dihitung
// dengan: tinggi_badan = TINGGI_SENSOR_DARI_LANTAI - jarak_sensor_ke_kepala.
const float TINGGI_SENSOR_DARI_LANTAI = 193; // cm

const float TINGGI_JARAK_MIN = 20; // cm, di bawah ini bacaan dianggap tidak valid

// Jarak sensor->objek secara fisik tidak mungkin lebih jauh dari tinggi
// sensor itu sendiri (kalau lurus ke bawah, paling jauh ya kena lantai).
// Dikasih sedikit toleransi (+10cm) untuk margin ketidaktepatan pemasangan.
const float TINGGI_JARAK_MAX = TINGGI_SENSOR_DARI_LANTAI + 10; // cm

// Lompatan antar-bacaan yang lebih besar dari ini (dalam satu siklus baca)
// dianggap pantulan/echo salah (noise), bukan perubahan tinggi sungguhan.
const float TINGGI_LONCATAN_MAX = 30; // cm

// Kalau lompatan besar terjadi BERTURUT-TURUT sebanyak ini, dianggap
// perubahan asli (misal orang baru masuk/keluar posisi ukur), bukan noise
// sesaat -> titik acuan (anchor) di-update ke nilai baru.
int tinggiPenolakanBerturut = 0;
const int TINGGI_PENOLAKAN_MAX = 3;

unsigned long lastBacaTinggi = 0;

// Berat & tinggi selalu dibaca realtime cepat di 0.5 detik, tidak peduli
// sedang mode ukur perut atau tidak.
const unsigned long INTERVAL_BACA_TINGGI = 500;

// ---------------- OPTOCOUPLER (KELILING) ----------------
#define SENSOR_PIN 2

float CM_PER_PULSA = 1.0;

unsigned long pulseCount = 0;

int sensorRawTerakhir = HIGH;
int sensorStabil = HIGH;
unsigned long waktuSensorBerubah = 0;
const unsigned long DEBOUNCE_MS = 15; // naikkan jika masih kebaca terus saat diam

float keliling_cm = 0;

bool pengukuranAktif = false;

unsigned long lastPrint = 0;

// LCD/serial print juga diperlambat saat ukur perut, supaya operasi I2C
// (yang blocking) tidak sering mengganggu polling pulsa.
const unsigned long INTERVAL_PRINT_NORMAL = 150;
const unsigned long INTERVAL_PRINT_UKUR   = 150;

// ---------------- TOMBOL FISIK (SIMULASI PERINTAH SERIAL) ----------------
// Tombol ke GND, pakai INPUT_PULLUP (tertekan = LOW).
const int BUTTON_S_PIN = 32; // sama seperti ketik 'S' di Serial Monitor
const int BUTTON_K_PIN = 33; // sama seperti ketik 'K' di Serial Monitor

const unsigned long DEBOUNCE_TOMBOL_MS = 250;

int stateTombolS = HIGH;
int stateTombolK = HIGH;
unsigned long waktuTombolSTerakhir = 0;
unsigned long waktuTombolKTerakhir = 0;


//====================================================
// WIFI
//====================================================
void hubungkanWiFi() {

  Serial.print("Menghubungkan ke WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi terhubung, IP ESP32: ");
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
// KIRIM DATA KE SERVER
//====================================================
void kirimDataKeServer() {

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi terputus, data tidak dikirim.");
    return;
  }

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"api_key\":\"" + String(API_KEY) + "\",";
  payload += "\"berat_badan\":" + String(beratHalus, 2) + ",";
  payload += "\"tinggi_badan\":" + String(tinggi_cm, 1) + ",";
  payload += "\"lingkar_perut\":" + String(keliling_cm, 1);
  payload += "}";

  int kodeRespon = http.POST(payload);

  if (kodeRespon > 0) {
    Serial.print("Kirim data OK, respon: ");
    Serial.println(http.getString());
  } else {
    Serial.print("Kirim data GAGAL, kode: ");
    Serial.println(kodeRespon);
  }

  http.end();
}

//====================================================
// BACA PULSA OPTOCOUPLER (POLLING + DEBOUNCE STABIL)
//====================================================
void updatePulsaKeliling() {

  if (!pengukuranAktif) return;

  int raw = digitalRead(SENSOR_PIN);

  if (raw != sensorRawTerakhir) {
    waktuSensorBerubah = millis();
    sensorRawTerakhir = raw;
  }

  if (millis() - waktuSensorBerubah > DEBOUNCE_MS) {

    if (raw != sensorStabil) {

      sensorStabil = raw;

      if (sensorStabil == LOW) {
        pulseCount++;
      }
    }
  }
}

//====================================================
// AKSI BERSAMA (DIPAKAI SERIAL COMMAND & TOMBOL FISIK)
//====================================================
void aksiMulaiUkurPerut() {

  resetKeliling();
  pengukuranAktif = true;

  // Paksa refresh LCD/serial & tinggi langsung di siklus berikutnya
  lastPrint = 0;
  lastBacaTinggi = 0;

  Serial.println(">>> MULAI UKUR LINGKAR PERUT (mode halus aktif) <<<");
}

void aksiKirimData() {

  Serial.println(">>> KIRIM DATA KE WEB <<<");
  kirimDataKeServer();
}

//====================================================
// RESET KELILING
//====================================================
void resetKeliling() {

  pulseCount = 0;
  sensorRawTerakhir = digitalRead(SENSOR_PIN);
  sensorStabil = sensorRawTerakhir;
  waktuSensorBerubah = millis();

  keliling_cm = 0;

  Serial.println(">>> KELILING DIRESET <<<");
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

  if (durasi == 0) {
    return tinggiSebelumnya;
  }

  float d = durasi * 0.0343 / 2.0;

  if (d < TINGGI_JARAK_MIN || d > TINGGI_JARAK_MAX) {
    return tinggiSebelumnya;
  }

  // Bacaan valid PERTAMA kali dipakai apa adanya sebagai titik awal,
  // tanpa smoothing dan tanpa filter loncatan (supaya tidak "ketarik" ke 0).
  if (!tinggiSudahValid) {
    tinggiSebelumnya = d;
    tinggiSudahValid = true;
    return d;
  }

  // Kalau lompatannya jauh lebih besar dari wajar dibanding bacaan
  // sebelumnya, kemungkinan besar itu pantulan/echo salah -> abaikan.
  // Tapi kalau ini terjadi berturut-turut, anggap perubahan asli dan
  // pindahkan anchor ke nilai baru supaya tidak nyangkut selamanya.
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

  Serial.println();
  Serial.println("Starting...");

  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Menghubungkan WiFi..");

  hubungkanWiFi();

  lcd.clear();

  // LOAD CELL
  LoadCell.begin();

  unsigned long stabilizingtime = 2000;
  boolean _tare = true;

  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag()) {

    Serial.println("Timeout HX711");

  } else {

    LoadCell.setCalFactor(calibrationValue);
    Serial.println("Load cell startup selesai.");
  }

  while (!LoadCell.update());

  berat = LoadCell.getData() - OFFSET_KALIBRASI;
  beratHalus = berat; // inisialisasi biar LCD langsung tampil nilai benar, tidak ramp dari 0

  // ULTRASONIK
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  // OPTOCOUPLER
  pinMode(SENSOR_PIN, INPUT);
  sensorRawTerakhir = digitalRead(SENSOR_PIN);
  sensorStabil = sensorRawTerakhir;

  // TOMBOL FISIK
  pinMode(BUTTON_S_PIN, INPUT_PULLUP);
  pinMode(BUTTON_K_PIN, INPUT_PULLUP);
  stateTombolS = digitalRead(BUTTON_S_PIN);
  stateTombolK = digitalRead(BUTTON_K_PIN);

  Serial.println("==================================");
  Serial.println("S = Mulai ukur lingkar perut");
  Serial.println("R = Reset & Stop ukur lingkar");
  Serial.println("T = Tare Load Cell");
  Serial.println("K = Kirim data ke web sekarang");
  Serial.println("(Tombol fisik S/K juga tersedia)");
  Serial.println("==================================");
}

//====================================================
// LOOP
//====================================================
void loop() {

  unsigned long now = millis();

  // ================== LOAD CELL (BERAT) ==================
  // Pembacaan mentah tetap tiap loop (murah/cepat, HX711_ADC non-blocking).
  LoadCell.update();

  berat = LoadCell.getData() - OFFSET_KALIBRASI;

  if (fabs(berat) < BERAT_DEADBAND) {
    berat = 0;
  }

  // Nilai yang DITAMPILKAN dihaluskan. Saat idle alpha besar (responsif,
  // hampir sama dengan nilai mentah = real-time). Saat ukur perut alpha
  // kecil (perubahan dibuat pelan/halus) supaya LCD tidak "loncat-loncat"
  // walau update jarang, dan supaya loop lebih fokus polling pulsa.
  float alphaBerat = pengukuranAktif ? ALPHA_BERAT_UKUR : ALPHA_BERAT_NORMAL;
  beratHalus = (alphaBerat * berat) + ((1.0 - alphaBerat) * beratHalus);

  // ================== TINGGI ==================
  // Berat & tinggi selalu realtime cepat di 0.5 detik, tidak tergantung
  // mode ukur perut (hanya lingkar perut yang punya perilaku khusus saat 'S').
  if (now - lastBacaTinggi >= INTERVAL_BACA_TINGGI) {

    lastBacaTinggi = now;

    jarakSensor = bacaTinggi();
    tinggi_cm = TINGGI_SENSOR_DARI_LANTAI - jarakSensor;

    if (tinggi_cm < 0) {
      tinggi_cm = 0;
    }
  }

  // ================== KELILING (PRIORITAS SAAT UKUR PERUT) ==================
  // Dipanggil setiap iterasi loop tanpa syarat interval, supaya polling
  // pin optocoupler sesering mungkin -> pulsa terbaca akurat.
  updatePulsaKeliling();
  keliling_cm = pulseCount * CM_PER_PULSA;

  // ==================================================
  // SERIAL COMMAND
  // ==================================================
  if (Serial.available()) {

    char cmd = Serial.read();

    if (cmd == 'S' || cmd == 's') {

      aksiMulaiUkurPerut();
    }

    else if (cmd == 'R' || cmd == 'r') {

      resetKeliling();
      pengukuranAktif = false;

      // Paksa refresh cepat langsung setelah selesai ukur
      lastPrint = 0;
      lastBacaTinggi = 0;

      Serial.println(">>> RESET & STOP UKUR LINGKAR PERUT (mode real-time aktif) <<<");
    }

    else if (cmd == 'T' || cmd == 't') {

      LoadCell.tareNoDelay();
    }

    else if (cmd == 'K' || cmd == 'k') {

      aksiKirimData();
    }
  }

  // ==================================================
  // TOMBOL FISIK (SIMULASI PERINTAH S DAN K)
  // ==================================================
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

  if (LoadCell.getTareStatus()) {

    Serial.println("Tare load cell selesai.");
  }

  // ==================================================
  // TAMPILKAN DATA (interval berbeda tergantung mode)
  // ==================================================
  unsigned long intervalPrintSaatIni =
      pengukuranAktif ? INTERVAL_PRINT_UKUR : INTERVAL_PRINT_NORMAL;

  if (now - lastPrint >= intervalPrintSaatIni) {

    lastPrint = now;

    Serial.print("Berat: ");
    Serial.print(beratHalus, 2);
    Serial.print(" kg");

    Serial.print(" | Tinggi: ");
    Serial.print(tinggi_cm, 1);
    Serial.print(" cm");

    Serial.print(" (jarak sensor: ");
    Serial.print(jarakSensor, 1);
    Serial.print(" cm)");

    if (pengukuranAktif) {

      Serial.print(" | Keliling: ");
      Serial.print(keliling_cm, 1);
      Serial.print(" cm");
    }

    Serial.println();

    tampilkanLCD();
  }

  // Pengiriman ke web TIDAK lagi otomatis berkala.
  // Kirim hanya saat tombol 'K' ditekan di Serial Monitor (lihat blok SERIAL COMMAND).
}
