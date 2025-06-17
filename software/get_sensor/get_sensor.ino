#include <WiFi.h>
#include <esp_now.h>

// Data structure to match sender
typedef struct struct_message {
  float heading;
  float gyroX;
  float gyroY;
  float gyroZ;
  unsigned long timestamp;
} struct_message;

struct_message receivedData;

// Movement analysis variables
struct MovementPattern {
  String name;
  float minX, maxX, avgX;
  float minY, maxY, avgY;
  float minZ, maxZ, avgZ;
  float sumX, sumY, sumZ;
  int sampleCount;
  unsigned long startTime;
  bool active;
};

MovementPattern currentAnalysis;
MovementPattern completedPatterns[20];
int patternCount = 0;

// Analysis control
bool analysisMode = false;
unsigned long analysisStartTime = 0;
const unsigned long ANALYSIS_DURATION = 10000; // 10 seconds

// Data logging
unsigned long lastLogTime = 0;
const unsigned long LOG_INTERVAL = 500; // Log every 500ms

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("🔍========================================🔍");
  Serial.println("🎯   GYROSCOPE DATA ANALYZER TOOL   🎯");
  Serial.println("🔍========================================🔍");
  Serial.printf("📅 Date: 2025-06-17 10:21:16 UTC\n");
  Serial.printf("👨‍💻 User: AmrEslim\n");
  Serial.printf("🔧 Purpose: Analyze compass-gyro data patterns\n");
  Serial.println("🔍========================================🔍");
  
  // Initialize WiFi for ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  
  Serial.printf("📻 Receiver MAC: %s\n", WiFi.macAddress().c_str());
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW initialization failed!");
    return;
  }
  
  Serial.println("✅ ESP-NOW initialized successfully");
  esp_now_register_recv_cb(OnDataReceived);
  
  Serial.println("🔍========================================🔍");
  Serial.println("🎯 READY FOR ANALYSIS!");
  Serial.println("📋 COMMANDS:");
  Serial.println("   Type 'forward' + Enter to analyze forward tilt");
  Serial.println("   Type 'backward' + Enter to analyze backward tilt");
  Serial.println("   Type 'left' + Enter to analyze left tilt");
  Serial.println("   Type 'right' + Enter to analyze right tilt");
  Serial.println("   Type 'shake' + Enter to analyze shake motion");
  Serial.println("   Type 'still' + Enter to analyze still position");
  Serial.println("   Type 'summary' + Enter to show all results");
  Serial.println("   Type 'clear' + Enter to clear all data");
  Serial.println("🔍========================================🔍");
}

void OnDataReceived(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  if (len == sizeof(receivedData)) {
    memcpy(&receivedData, incomingData, sizeof(receivedData));
    
    // Process data if in analysis mode
    if (analysisMode) {
      processAnalysisData(receivedData.gyroX, receivedData.gyroY, receivedData.gyroZ);
    }
    
    // Log data periodically
    if (millis() - lastLogTime >= LOG_INTERVAL) {
      logCurrentData();
      lastLogTime = millis();
    }
  }
}

void processAnalysisData(float gx, float gy, float gz) {
  if (!currentAnalysis.active) return;
  
  // First sample - initialize min/max
  if (currentAnalysis.sampleCount == 0) {
    currentAnalysis.minX = currentAnalysis.maxX = gx;
    currentAnalysis.minY = currentAnalysis.maxY = gy;
    currentAnalysis.minZ = currentAnalysis.maxZ = gz;
    currentAnalysis.sumX = currentAnalysis.sumY = currentAnalysis.sumZ = 0;
  }
  
  // Update statistics
  if (gx < currentAnalysis.minX) currentAnalysis.minX = gx;
  if (gx > currentAnalysis.maxX) currentAnalysis.maxX = gx;
  if (gy < currentAnalysis.minY) currentAnalysis.minY = gy;
  if (gy > currentAnalysis.maxY) currentAnalysis.maxY = gy;
  if (gz < currentAnalysis.minZ) currentAnalysis.minZ = gz;
  if (gz > currentAnalysis.maxZ) currentAnalysis.maxZ = gz;
  
  currentAnalysis.sumX += gx;
  currentAnalysis.sumY += gy;
  currentAnalysis.sumZ += gz;
  currentAnalysis.sampleCount++;
  
  // Progress indicator
  if (currentAnalysis.sampleCount % 10 == 0) {
    unsigned long elapsed = millis() - currentAnalysis.startTime;
    float progress = (elapsed / (float)ANALYSIS_DURATION) * 100;
    Serial.printf("📊 %s: %.1f%% complete (Samples: %d)\n", 
                  currentAnalysis.name.c_str(), progress, currentAnalysis.sampleCount);
  }
  
  // Check if analysis is complete
  if (millis() - currentAnalysis.startTime >= ANALYSIS_DURATION) {
    finishAnalysis();
  }
}

void startAnalysis(String movementName) {
  if (analysisMode) {
    Serial.println("⚠️ Analysis already in progress! Wait for completion.");
    return;
  }
  
  Serial.println("🔍========================================🔍");
  Serial.printf("🎯 STARTING ANALYSIS: %s\n", movementName.c_str());
  Serial.println("📊 PERFORM THE MOVEMENT NOW!");
  Serial.printf("⏱️ Analysis duration: %d seconds\n", ANALYSIS_DURATION / 1000);
  Serial.println("🔍========================================🔍");
  
  analysisMode = true;
  currentAnalysis.name = movementName;
  currentAnalysis.active = true;
  currentAnalysis.startTime = millis();
  currentAnalysis.sampleCount = 0;
}

void finishAnalysis() {
  if (!currentAnalysis.active) return;
  
  // Calculate averages
  if (currentAnalysis.sampleCount > 0) {
    currentAnalysis.avgX = currentAnalysis.sumX / currentAnalysis.sampleCount;
    currentAnalysis.avgY = currentAnalysis.sumY / currentAnalysis.sampleCount;
    currentAnalysis.avgZ = currentAnalysis.sumZ / currentAnalysis.sampleCount;
  }
  
  // Print results
  Serial.println("🔍========================================🔍");
  Serial.printf("📋 ANALYSIS COMPLETE: %s\n", currentAnalysis.name.c_str());
  Serial.printf("📊 Total samples: %d\n", currentAnalysis.sampleCount);
  Serial.printf("⏱️ Duration: %.1f seconds\n", (millis() - currentAnalysis.startTime) / 1000.0);
  Serial.println();
  
  Serial.println("🔄 X-AXIS (Pitch) RESULTS:");
  Serial.printf("   Min: %8.3f°/s\n", currentAnalysis.minX);
  Serial.printf("   Max: %8.3f°/s\n", currentAnalysis.maxX);
  Serial.printf("   Avg: %8.3f°/s\n", currentAnalysis.avgX);
  Serial.printf("   Range: %6.3f°/s\n", currentAnalysis.maxX - currentAnalysis.minX);
  Serial.printf("   Abs Max: %6.3f°/s\n", max(abs(currentAnalysis.minX), abs(currentAnalysis.maxX)));
  
  Serial.println();
  Serial.println("🔄 Y-AXIS (Roll) RESULTS:");
  Serial.printf("   Min: %8.3f°/s\n", currentAnalysis.minY);
  Serial.printf("   Max: %8.3f°/s\n", currentAnalysis.maxY);
  Serial.printf("   Avg: %8.3f°/s\n", currentAnalysis.avgY);
  Serial.printf("   Range: %6.3f°/s\n", currentAnalysis.maxY - currentAnalysis.minY);
  Serial.printf("   Abs Max: %6.3f°/s\n", max(abs(currentAnalysis.minY), abs(currentAnalysis.maxY)));
  
  Serial.println();
  Serial.println("🔄 Z-AXIS (Yaw) RESULTS:");
  Serial.printf("   Min: %8.3f°/s\n", currentAnalysis.minZ);
  Serial.printf("   Max: %8.3f°/s\n", currentAnalysis.maxZ);
  Serial.printf("   Avg: %8.3f°/s\n", currentAnalysis.avgZ);
  Serial.printf("   Range: %6.3f°/s\n", currentAnalysis.maxZ - currentAnalysis.minZ);
  Serial.printf("   Abs Max: %6.3f°/s\n", max(abs(currentAnalysis.minZ), abs(currentAnalysis.maxZ)));
  
  Serial.println();
  Serial.println("💡 SUGGESTED THRESHOLDS:");
  float thresholdX = max(abs(currentAnalysis.minX), abs(currentAnalysis.maxX)) * 0.6;
  float thresholdY = max(abs(currentAnalysis.minY), abs(currentAnalysis.maxY)) * 0.6;
  float thresholdZ = max(abs(currentAnalysis.minZ), abs(currentAnalysis.maxZ)) * 0.6;
  
  Serial.printf("   X-Axis: %.2f°/s (60%% of max)\n", thresholdX);
  Serial.printf("   Y-Axis: %.2f°/s (60%% of max)\n", thresholdY);
  Serial.printf("   Z-Axis: %.2f°/s (60%% of max)\n", thresholdZ);
  Serial.println("🔍========================================🔍");
  
  // Store completed analysis
  if (patternCount < 20) {
    completedPatterns[patternCount] = currentAnalysis;
    patternCount++;
  }
  
  // Reset for next analysis
  analysisMode = false;
  currentAnalysis.active = false;
}

void logCurrentData() {
  Serial.printf("📡 Live Data - Heading: %6.2f° | Gyro X: %6.3f°/s | Y: %6.3f°/s | Z: %6.3f°/s\n",
                receivedData.heading, receivedData.gyroX, receivedData.gyroY, receivedData.gyroZ);
}

void printSummary() {
  Serial.println("📋========================================📋");
  Serial.println("📊 COMPLETE ANALYSIS SUMMARY");
  Serial.println("📋========================================📋");
  
  if (patternCount == 0) {
    Serial.println("⚠️ No analysis data available. Run some movement analyses first!");
    return;
  }
  
  Serial.printf("📊 Total patterns analyzed: %d\n\n", patternCount);
  
  // Print table header
  Serial.println("Movement      | X-Axis Range  | Y-Axis Range  | Z-Axis Range  | Suggested Thresholds");
  Serial.println("--------------|---------------|---------------|---------------|---------------------");
  
  for (int i = 0; i < patternCount; i++) {
    MovementPattern &p = completedPatterns[i];
    float thresholdX = max(abs(p.minX), abs(p.maxX)) * 0.6;
    float thresholdY = max(abs(p.minY), abs(p.maxY)) * 0.6;
    float thresholdZ = max(abs(p.minZ), abs(p.maxZ)) * 0.6;
    
    Serial.printf("%-13s | %5.2f to %5.2f | %5.2f to %5.2f | %5.2f to %5.2f | X:%.2f Y:%.2f Z:%.2f\n",
                  p.name.c_str(),
                  p.minX, p.maxX,
                  p.minY, p.maxY,
                  p.minZ, p.maxZ,
                  thresholdX, thresholdY, thresholdZ);
  }
  
  // Calculate optimal thresholds across all movements
  Serial.println("\n💡 OPTIMAL THRESHOLDS FOR SNAKE ROBOT:");
  
  float maxThresholdX = 0, maxThresholdY = 0, maxThresholdZ = 0;
  for (int i = 0; i < patternCount; i++) {
    MovementPattern &p = completedPatterns[i];
    float thresholdX = max(abs(p.minX), abs(p.maxX)) * 0.6;
    float thresholdY = max(abs(p.minY), abs(p.maxY)) * 0.6;
    float thresholdZ = max(abs(p.minZ), abs(p.maxZ)) * 0.6;
    
    if (thresholdX > maxThresholdX) maxThresholdX = thresholdX;
    if (thresholdY > maxThresholdY) maxThresholdY = thresholdY;
    if (thresholdZ > maxThresholdZ) maxThresholdZ = thresholdZ;
  }
  
  Serial.printf("   GYRO_THRESHOLD_X = %.2f;  // degrees/sec\n", maxThresholdX);
  Serial.printf("   GYRO_THRESHOLD_Y = %.2f;  // degrees/sec\n", maxThresholdY);
  Serial.printf("   GYRO_THRESHOLD_Z = %.2f;  // degrees/sec\n", maxThresholdZ);
  
  Serial.println("📋========================================📋");
}

void clearAllData() {
  patternCount = 0;
  analysisMode = false;
  currentAnalysis.active = false;
  Serial.println("🗑️ All analysis data cleared!");
}

void processSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "forward") {
      startAnalysis("FORWARD_TILT");
    } else if (command == "backward") {
      startAnalysis("BACKWARD_TILT");
    } else if (command == "left") {
      startAnalysis("LEFT_TILT");
    } else if (command == "right") {
      startAnalysis("RIGHT_TILT");
    } else if (command == "shake") {
      startAnalysis("SHAKE_MOTION");
    } else if (command == "still") {
      startAnalysis("STILL_POSITION");
    } else if (command == "summary") {
      printSummary();
    } else if (command == "clear") {
      clearAllData();
    } else if (command == "help") {
      Serial.println("📋 Available commands: forward, backward, left, right, shake, still, summary, clear, help");
    } else if (command.length() > 0) {
      Serial.println("❓ Unknown command: " + command + " (type 'help' for commands)");
    }
  }
}

void loop() {
  processSerialCommands();
  yield();
}