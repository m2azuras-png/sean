#include <HX711_ADC.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ---------------- LOAD CELL ----------------
const int HX711_dout = 25;
const int HX711_sck  = 26;
HX711_ADC LoadCell(HX711_dout, HX711_sck);
float calibrationValue = 28086.2; // Dikalibrasi ulang: 25865.5 x (99.90/92) karena hasil baca sebelumnya 99.9kg utk beban 92kg asli

// ---------------- TARGET ESP32 UTAMA ----------------
// GANTI dengan MAC Address ESP32 Utama (lihat Serial Monitor ESP32 Utama saat startup,
// baris "MAC Address ESP32 Utama : xx:xx:xx:xx:xx:xx")
uint8_t alamatUtama[] = {0xB4, 0xBF, 0xE9, 0x61, 0x68, 0x04};

// GANTI dengan channel WiFi ESP32 Utama (lihat baris "WiFi Channel saat ini : N"
// di Serial Monitor ESP32 Utama). Harus SAMA PERSIS di kedua board.
#define WIFI_CHANNEL 9

// ---------------- ESP-NOW ----------------
// Struktur data yang dikirim
typedef struct struct_message {
  float berat;
} struct_message;

struct_message myData;

esp_now_peer_info_t peerInfo;

unsigned long lastTime = 0;
const unsigned long timerDelay = 100; // Kirim data tiap 100ms

void setup() {
  Serial.begin(115200);

  // Tidak perlu konek ke router sama sekali. Cukup aktifkan mode STA
  // dan paksa channel WiFi-nya sama dengan channel ESP32 Utama.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  Serial.print("Channel WiFi dipaksa ke: ");
  Serial.println(WIFI_CHANNEL);

  // Inisialisasi ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error inisialisasi ESP-NOW");
    return;
  }

  // Daftarkan peer langsung ke MAC Address ESP32 Utama (unicast, bukan broadcast)
  memcpy(peerInfo.peer_addr, alamatUtama, 6);
  peerInfo.channel = WIFI_CHANNEL;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Gagal menambah peer");
    return;
  }
  Serial.println("Peer ESP32 Utama terdaftar. Siap kirim data.");

  // Mulai Loadcell
  LoadCell.begin();
  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout HX711!");
  } else {
    LoadCell.setCalFactor(calibrationValue);
    Serial.println("HX711 Siap.");
  }
}

void loop() {
  // Non-blocking update HX711
  LoadCell.update();

  // Kirim data secara berkala
  if (millis() - lastTime > timerDelay) {
    myData.berat = LoadCell.getData();

    // Tampilkan berat di Serial Monitor
    Serial.print("Berat: ");
    Serial.print(myData.berat, 2);
    Serial.println(" kg");

    // Kirim via ESP-NOW langsung ke MAC Address ESP32 Utama
    esp_now_send(alamatUtama, (uint8_t *) &myData, sizeof(myData));

    lastTime = millis();
  }
}