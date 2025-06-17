#include <WiFi.h>
#include <esp_now.h>
#include <QMC5883LCompass.h>

QMC5883LCompass compass;

// ESP-NOW setup
uint8_t snakeAddress[] = {0x34, 0x94, 0x54, 0xAA, 0x4E, 0x70}; // Snake MAC address

// Command structure to send to snake
typedef struct struct_command {
  String command;
  unsigned long timestamp;
  int intensity; // For rattle intensity or movement speed
} struct_command;

struct_command snakeCommand;

// Calibration variables
int baseX = 0, baseY = 0, baseZ = 0;
bool isCalibrated = false;

// Movement detection thresholds (from your working logic)
const int LEFT_THRESHOLD_Y = 800;   
const int RIGHT_THRESHOLD_Y = 800;  // Same as left since we use direction logic
const int RATTLE_THRESHOLD_X = 1500;

// Detection window variables
struct MovementWindow {
  int minX, maxX, minY, maxY, minZ, maxZ;
  unsigned long startTime;
  bool active;
  int lastX, lastY, lastZ;
};

MovementWindow window = {0, 0, 0, 0, 0, 0, 0, false, 0, 0, 0};

const unsigned long DETECTION_WINDOW = 1000; // 1 second window
const int MOVEMENT_START_THRESHOLD = 100;

// ESP-NOW callback
void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("📡 Command sent: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "✅ SUCCESS" : "❌ FAILED");
}

void sendCommandToSnake(String cmd, int intensity = 1) {
  snakeCommand.command = cmd;
  snakeCommand.timestamp = millis();
  snakeCommand.intensity = intensity;
  
  Serial.printf("📤 Sending command: %s (intensity: %d)\n", cmd.c_str(), intensity);
  
  esp_err_t result = esp_now_send(snakeAddress, (uint8_t *)&snakeCommand, sizeof(snakeCommand));
  
  if (result == ESP_OK) {
    Serial.println("✅ Command transmission initiated");
  } else {
    Serial.printf("❌ Send error: %d\n", result);
  }
}

void calibrateCompass() {
  Serial.println("🔧 ========================================");
  Serial.println("🔧 CALIBRATING COMPASS - KEEP STILL!");
  Serial.println("🔧 ========================================");
  
  long sumX = 0, sumY = 0, sumZ = 0;
  const int samples = 50;
  
  for (int i = 0; i < samples; i++) {
    compass.read();
    sumX += compass.getX();
    sumY += compass.getY();
    sumZ += compass.getZ();
    
    if (i % 10 == 0) {
      Serial.printf("📊 Calibration: %d/%d\n", i, samples);
    }
    delay(50);
  }
  
  baseX = sumX / samples;
  baseY = sumY / samples;
  baseZ = sumZ / samples;
  isCalibrated = true;
  
  Serial.println("✅ CALIBRATION COMPLETE!");
  Serial.printf("📊 Base: X=%d, Y=%d, Z=%d\n", baseX, baseY, baseZ);
  Serial.println("🔧 ========================================");
}

String detectMovement(int calX, int calY, int calZ) {
  unsigned long now = millis();
  
  // Check if significant movement started
  if (!window.active) {
    int deltaX = abs(calX - window.lastX);
    int deltaY = abs(calY - window.lastY);
    int deltaZ = abs(calZ - window.lastZ);
    
    if (deltaX > MOVEMENT_START_THRESHOLD || deltaY > MOVEMENT_START_THRESHOLD || deltaZ > MOVEMENT_START_THRESHOLD) {
      // Start detection window
      window.active = true;
      window.startTime = now;
      window.minX = window.maxX = calX;
      window.minY = window.maxY = calY;
      window.minZ = window.maxZ = calZ;
      
      Serial.println("🎯 Movement detection started...");
    }
  }
  
  if (window.active) {
    // Update ranges
    window.minX = min(window.minX, calX);
    window.maxX = max(window.maxX, calX);
    window.minY = min(window.minY, calY);
    window.maxY = max(window.maxY, calY);
    window.minZ = min(window.minZ, calZ);
    window.maxZ = max(window.maxZ, calZ);
    
    // Check if window is complete
    if (now - window.startTime >= DETECTION_WINDOW) {
      int rangeX = window.maxX - window.minX;
      int rangeY = window.maxY - window.minY;
      int rangeZ = window.maxZ - window.minZ;
      
      Serial.printf("🔍 Analysis: X-range=%d, Y-range=%d, Z-range=%d\n", 
                    rangeX, rangeY, rangeZ);
      
      String result = "NONE";
      
      // Check rattle first (highest priority)
      if (rangeX > RATTLE_THRESHOLD_X) {
        result = "RATTLE";
        Serial.println("🐍 RATTLE DETECTED!");
      }
      // Check left/right movements
      else if (rangeY > LEFT_THRESHOLD_Y) {
        if (window.minY < -400) { // Left movement goes into negative Y territory
          result = "LEFT";
          Serial.printf("⬅️ LEFT MOVEMENT DETECTED! (Y-range=%d, min=%d)\n", rangeY, window.minY);
        } else if (window.maxY > 800) { // Right movement stays in positive Y territory
          result = "RIGHT";
          Serial.printf("➡️ RIGHT MOVEMENT DETECTED! (Y-range=%d, max=%d)\n", rangeY, window.maxY);
        }
      }
      else {
        Serial.printf("❌ Movement too small - ignored (X=%d, Y=%d, Z=%d)\n", 
                     rangeX, rangeY, rangeZ);
      }
      
      // Reset window
      window.active = false;
      return result;
    }
  }
  
  // Update last values
  window.lastX = calX;
  window.lastY = calY; 
  window.lastZ = calZ;
  
  return "DETECTING";
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("🧭========================================🧭");
  Serial.println("🚀   COMPASS SNAKE CONTROLLER v1.0      🚀");
  Serial.println("🧭========================================🧭");
  Serial.printf("📅 Date: 2025-06-17 12:31:30 UTC\n");
  Serial.printf("👨‍💻 Developer: AmrEslim\n");
  Serial.printf("🔧 Version: ESP-NOW Snake Control\n");
  Serial.println("🧭========================================🧭");
  
  // Step 1: Initialize WiFi for ESP-NOW
  Serial.println("📡 Step 1: Initializing WiFi for ESP-NOW...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("📻 Controller MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Step 2: Initialize ESP-NOW
  Serial.println("📶 Step 2: Initializing ESP-NOW...");
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init FAILED");
    return;
  }
  Serial.println("✅ ESP-NOW initialized");
  
  // Register send callback
  esp_now_register_send_cb(OnSent);
  
  // Add snake as peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, snakeAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("❌ Failed to add snake as peer");
    return;
  }
  Serial.printf("✅ Snake added as peer: %02X:%02X:%02X:%02X:%02X:%02X\n",
                snakeAddress[0], snakeAddress[1], snakeAddress[2],
                snakeAddress[3], snakeAddress[4], snakeAddress[5]);
  
  // Step 3: Initialize Compass
  Serial.println("🧭 Step 3: Initializing compass...");
  Wire.begin(21, 22);
  compass.init();
  Serial.println("✅ Compass initialized");
  
  // Step 4: Calibrate compass
  delay(2000);
  calibrateCompass();
  
  // Print control information
  Serial.println("🎯 SNAKE CONTROL COMMANDS:");
  Serial.printf("🎯 LEFT:   Y-range > %d AND Y goes negative (< -400)\n", LEFT_THRESHOLD_Y);
  Serial.printf("🎯 RIGHT:  Y-range > %d AND Y stays positive (> 800)\n", RIGHT_THRESHOLD_Y);
  Serial.printf("🎯 RATTLE: X-range > %d (highest priority)\n", RATTLE_THRESHOLD_X);
  Serial.println("🧭========================================🧭");
  
  Serial.println("🤖 SNAKE CONTROLLER READY!");
  Serial.println("   ⬅️ Tilt LEFT to turn snake left");  
  Serial.println("   ➡️ Tilt RIGHT to turn snake right");
  Serial.println("   🐍 SHAKE to make snake rattle");
  Serial.println("🧭========================================🧭");
  
  // Send initial "ready" command to snake
  delay(1000);
  sendCommandToSnake("READY", 1);
}

void loop() {
  // Read compass
  compass.read();
  int rawX = compass.getX();
  int rawY = compass.getY(); 
  int rawZ = compass.getZ();
  
  // Apply calibration
  int calX = rawX - baseX;
  int calY = rawY - baseY;
  int calZ = rawZ - baseZ;
  
  // Detect movement
  String movement = detectMovement(calX, calY, calZ);
  
  // Send commands to snake based on detected movement
  if (movement == "LEFT") {
    Serial.println("🎮 ✅ SENDING LEFT COMMAND TO SNAKE!");
    sendCommandToSnake("LEFT", 1);
    delay(2000); // Cooldown to prevent spam
  }
  else if (movement == "RIGHT") {
    Serial.println("🎮 ✅ SENDING RIGHT COMMAND TO SNAKE!");
    sendCommandToSnake("RIGHT", 1);
    delay(2000); // Cooldown to prevent spam
  }
  else if (movement == "RATTLE") {
    Serial.println("🎮 ✅ SENDING RATTLE COMMAND TO SNAKE!");
    // Calculate rattle intensity based on movement strength
    int intensity = map(window.maxX - window.minX, RATTLE_THRESHOLD_X, 3000, 1, 5);
    intensity = constrain(intensity, 1, 5);
    sendCommandToSnake("RATTLE", intensity);
    delay(3000); // Longer cooldown for rattle
  }
  
  // Status debug output
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 3000) { // Every 3 seconds
    Serial.printf("📊 Status: X=%d, Y=%d, Z=%d | Window: %s\n", 
                  calX, calY, calZ, window.active ? "ACTIVE" : "IDLE");
    if (window.active) {
      int elapsed = millis() - window.startTime;
      Serial.printf("📊 Window: %dms elapsed, ranges X=%d Y=%d Z=%d\n",
                    elapsed, window.maxX - window.minX, 
                    window.maxY - window.minY, window.maxZ - window.minZ);
    }
    lastDebug = millis();
  }
  
  delay(50); // 20Hz sampling rate
}