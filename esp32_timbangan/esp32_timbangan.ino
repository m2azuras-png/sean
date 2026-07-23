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
const char* WIFI_SSID     = "NAMA_WIFI_KAMU";
const char* WIFI_PASSWORD = "PASSWORD_WIFI_KAMU";

// Ganti sesuai IP lokal komputer XAMPP (cek dengan ipconfig)
const char* SERVER_URL = "http://192.168.1.196/Timbangan/api_sensor.php";
const char* API_KEY    = "ubah-kunci-ini";

unsigned long lastKirim = 0;
const unsigned long INTERVAL_KIRIM = 3000; // kirim tiap 3 detik

// ---------------- LOAD CELL (BERAT) ----------------
const int HX711_dout = 4;
const int HX711_sck  = 15;

HX711_ADC LoadCell(HX711_dout, HX711_sck);

float calibrationValue = 16368.0;

float berat = 0;
const float OFFSET_KALIBRASI = 0.0; // biarkan 0, tare otomatis di setup() yang menzerokan berat
const float BERAT_DEADBAND = 0.10; // kg, di bawah ini dianggap 0 (tidak ada beban)

// ---------------- ULTRASONIK JSN-SR04T (TINGGI) ----------------
const int TRIG_PIN = 18;
const int ECHO_PIN = 19;   // Ganti sesuai wiring

const unsigned long TIMEOUT_ECHO = 30000UL;

float tinggi_cm = 0;
float tinggiSebelumnya = 0;

unsigned long lastBacaTinggi = 0;
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
const unsigned long INTERVAL_PRINT = 500;

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

  tulisBarisLCD(0, "Berat : " + String(berat, 2) + " kg");
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
  payload += "\"berat_badan\":" + String(berat, 2) + ",";
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

  if (d < 20 || d > 450) {
    return tinggiSebelumnya;
  }

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

  // ULTRASONIK
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);

  // OPTOCOUPLER
  pinMode(SENSOR_PIN, INPUT);
  sensorRawTerakhir = digitalRead(SENSOR_PIN);
  sensorStabil = sensorRawTerakhir;

  Serial.println("==================================");
  Serial.println("S = Mulai ukur lingkar perut");
  Serial.println("R = Reset & Stop ukur lingkar");
  Serial.println("T = Tare Load Cell");
  Serial.println("==================================");
}

//====================================================
// LOOP
//====================================================
void loop() {

  unsigned long now = millis();

  // Update HX711
  LoadCell.update();

  berat = LoadCell.getData() - OFFSET_KALIBRASI;

  if (fabs(berat) < BERAT_DEADBAND) {
    berat = 0;
  }

  // Baca tinggi
  if (now - lastBacaTinggi >= INTERVAL_BACA_TINGGI) {

    lastBacaTinggi = now;

    tinggi_cm = bacaTinggi();
  }

  // Baca & hitung keliling
  updatePulsaKeliling();
  keliling_cm = pulseCount * CM_PER_PULSA;

  // ==================================================
  // SERIAL COMMAND
  // ==================================================
  if (Serial.available()) {

    char cmd = Serial.read();

    if (cmd == 'S' || cmd == 's') {

      resetKeliling();
      pengukuranAktif = true;

      Serial.println(">>> MULAI UKUR LINGKAR PERUT <<<");
    }

    else if (cmd == 'R' || cmd == 'r') {

      resetKeliling();
      pengukuranAktif = false;

      Serial.println(">>> RESET & STOP UKUR LINGKAR PERUT <<<");
    }

    else if (cmd == 'T' || cmd == 't') {

      LoadCell.tareNoDelay();
    }
  }

  if (LoadCell.getTareStatus()) {

    Serial.println("Tare load cell selesai.");
  }

  // ==================================================
  // TAMPILKAN DATA
  // ==================================================
  if (now - lastPrint >= INTERVAL_PRINT) {

    lastPrint = now;

    Serial.print("Berat: ");
    Serial.print(berat, 2);
    Serial.print(" kg");

    Serial.print(" | Tinggi: ");
    Serial.print(tinggi_cm, 1);
    Serial.print(" cm");

    if (pengukuranAktif) {

      Serial.print(" | Keliling: ");
      Serial.print(keliling_cm, 1);
      Serial.print(" cm");
    }

    Serial.println();

    tampilkanLCD();
  }

  // ==================================================
  // KIRIM DATA KE SERVER TIAP INTERVAL
  // ==================================================
  if (now - lastKirim >= INTERVAL_KIRIM) {

    lastKirim = now;

    kirimDataKeServer();
  }
}
