#include <WiFi.h>
#include <esp_now.h>
#include <QMC5883LCompass.h>

QMC5883LCompass compass;

// Updated struct to match receiver expectations
typedef struct struct_message {
  float heading;
  float gyroX;
  float gyroY;
  float gyroZ;
  unsigned long timestamp;
} struct_message;

struct_message msg;

uint8_t receiverAddress[] = {0x34, 0x94, 0x54, 0xAA, 0x4E, 0x70}; // Snake

// Variables for simulated gyro data from compass movements
float lastHeading = 0;
int lastX = 0, lastY = 0, lastZ = 0;
unsigned long lastTime = 0;
bool firstReading = true;

// Smoothing filter for gyro simulation
float gyroX_filtered = 0, gyroY_filtered = 0, gyroZ_filtered = 0;
const float FILTER_ALPHA = 0.3; // Smoothing factor (0.1 = heavy smoothing, 0.9 = light smoothing)

void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("📡 Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "✅ SUCCESS" : "❌ FAILED");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("🧭========================================🧭");
  Serial.println("🚀   QMC5883L COMPASS SENDER STARTING   🚀");
  Serial.println("🧭========================================🧭");
  Serial.printf("📅 Date: 2025-06-17 09:37 UTC\n");
  Serial.printf("👨‍💻 Developer: AmrEslim\n");
  Serial.printf("🔧 Version: Compass + Simulated Gyro\n");
  Serial.println("🧭========================================🧭");
  
  // Step 1: WiFi
  Serial.println("📡 Step 1: Initializing WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("📻 Sender MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Step 2: I2C for compass
  Serial.println("🔧 Step 2: Initializing I2C...");
  Wire.begin(21, 22);  // SDA=21, SCL=22 (standard ESP32 pins)
  Serial.println("✅ I2C initialized on pins 21(SDA), 22(SCL)");
  
  // Step 3: Initialize compass
  Serial.println("🧭 Step 3: Initializing QMC5883L compass...");
  compass.init();
  Serial.println("✅ Compass initialized successfully");
  
  // Step 4: ESP-NOW setup
  Serial.println("📶 Step 4: Initializing ESP-NOW...");
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init FAILED");
    return;
  }
  Serial.println("✅ ESP-NOW initialized");

  esp_now_register_send_cb(OnSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("❌ Failed to add peer");
    return;
  }
  
  Serial.println("🧭========================================🧭");
  Serial.println("🤖 COMPASS SENDER READY FOR SNAKE CONTROL! 🤖");
  Serial.printf("📦 Data packet size: %d bytes\n", sizeof(msg));
  Serial.println("🎯 Simulating gyroscope from compass movements");
  Serial.println("🧭========================================🧭");
  
  lastTime = millis();
}

void loop() {
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - lastTime) / 1000.0; // Convert to seconds
  
  // Read compass data
  compass.read();
  int x = compass.getX();
  int y = compass.getY();
  int z = compass.getZ();
  
  // Calculate heading
  float heading = atan2((float)y, (float)x) * 180.0 / PI;
  if (heading < 0) heading += 360;
  
  // Simulate gyroscope data from compass field changes
  float simGyroX = 0, simGyroY = 0, simGyroZ = 0;
  
  if (!firstReading && deltaTime > 0) {
    // Calculate rate of change for each axis
    float deltaX = (x - lastX) / deltaTime;
    float deltaY = (y - lastY) / deltaTime;
    float deltaZ = (z - lastZ) / deltaTime;
    
    // Map compass field changes to gyroscope-like movements
    // Scale factors adjusted for realistic gyro ranges
    simGyroX = constrain(deltaY * 0.01, -200, 200);  // Pitch from Y-axis changes
    simGyroY = constrain(deltaX * 0.01, -200, 200);  // Roll from X-axis changes
    
    // Calculate yaw rate from heading change
    float headingChange = heading - lastHeading;
    
    // Handle wrap-around (359° to 1° should be 2°, not -358°)
    if (headingChange > 180) headingChange -= 360;
    if (headingChange < -180) headingChange += 360;
    
    simGyroZ = headingChange / deltaTime; // Yaw rate in degrees per second
    simGyroZ = constrain(simGyroZ, -300, 300); // Limit to realistic range
    
    // Apply smoothing filter to reduce noise
    gyroX_filtered = FILTER_ALPHA * simGyroX + (1 - FILTER_ALPHA) * gyroX_filtered;
    gyroY_filtered = FILTER_ALPHA * simGyroY + (1 - FILTER_ALPHA) * gyroY_filtered;
    gyroZ_filtered = FILTER_ALPHA * simGyroZ + (1 - FILTER_ALPHA) * gyroZ_filtered;
  } else {
    firstReading = false;
  }
  
  // Prepare message
  msg.heading = heading;
  msg.gyroX = gyroX_filtered;
  msg.gyroY = gyroY_filtered;
  msg.gyroZ = gyroZ_filtered;
  msg.timestamp = currentTime;
  
  // Debug output (every 10 transmissions to avoid spam)
  static int debugCounter = 0;
  if (debugCounter % 10 == 0) {
    Serial.println("🧭 ===== COMPASS DATA =====");
    Serial.printf("📍 Heading: %.2f°\n", heading);
    Serial.printf("🧲 Raw: X=%d, Y=%d, Z=%d\n", x, y, z);
    Serial.printf("🔄 Simulated Gyro: X=%.1f°/s, Y=%.1f°/s, Z=%.1f°/s\n", 
                  gyroX_filtered, gyroY_filtered, gyroZ_filtered);
    Serial.printf("⏱️ Delta time: %.3fs\n", deltaTime);
    Serial.println("🧭 ========================");
  }
  debugCounter++;
  
  // Send data to snake robot
  esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&msg, sizeof(msg));
  
  if (result != ESP_OK) {
    Serial.printf("❌ Send error: %d\n", result);
  }
  
  // Update for next loop
  lastHeading = heading;
  lastX = x;
  lastY = y;
  lastZ = z;
  lastTime = currentTime;
  
  delay(100);  // Send at 10Hz for responsive control
  yield();
}