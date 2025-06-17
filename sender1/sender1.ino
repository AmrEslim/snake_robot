#include <WiFi.h>
#include <esp_now.h>
#include <QMC5883LCompass.h>

QMC5883LCompass compass;

typedef struct struct_message {
  float heading;
} struct_message;

struct_message msg;

//uint8_t receiverAddress[] = {0xCC, 0xDB, 0xA7, 0x16, 0x23, 0x54};
// uint8_t receiverAddress[] = {0xB0, 0xA7, 0x32, 0x2F, 0xBC, 0x20}; // <-- esp
 uint8_t receiverAddress[] = {0x34, 0x94, 0x54, 0xAA, 0x4E, 0x70}; // <-- Snake

void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAILED");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== STARTING SENDER ===");
  
  // Step 1: WiFi
  Serial.println("Step 1: Initializing WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.println("WiFi initialized");
  
  // Step 2: Try different I2C pins
  Serial.println("Step 2: Initializing I2C with default pins...");
  Wire.begin();  // Use default pins (usually 21=SDA, 22=SCL)
  Serial.println("I2C initialized with default pins");
  
  // Alternative: Try common ESP32 pins
  // Wire.begin(21, 22);  // SDA=21, SCL=22 (most common)
  // Wire.begin(4, 5);    // SDA=4, SCL=5 (alternative)
  
  Serial.println("Step 3: Initializing compass...");
  compass.init();
  Serial.println("Compass initialized");
  
  // Rest of ESP-NOW setup...
  Serial.println("Step 4: Initializing ESP-NOW...");
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init FAILED");
    return;
  }
  Serial.println("ESP-NOW initialized");

  esp_now_register_send_cb(OnSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("=== SETUP COMPLETE ===");
}

void loop() {
  compass.read();
  int x = compass.getX();
  int y = compass.getY();
  float heading = atan2((float)y, (float)x) * 180.0 / PI;
  if (heading < 0) heading += 360;

  msg.heading = heading;
  Serial.printf("Sending heading: %.2f\n", heading);
  
  esp_now_send(receiverAddress, (uint8_t *)&msg, sizeof(msg));
  delay(1000);
  yield();
}