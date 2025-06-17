#include <QMC5883LCompass.h>

QMC5883LCompass compass;

// Calibration variables
int baseX = 0, baseY = 0, baseZ = 0;
bool isCalibrated = false;

// Movement detection based on YOUR actual data - FIXED LEFT DETECTION
const int LEFT_THRESHOLD_Y = 800;   // Left: Y-axis range was 1167
const int RIGHT_THRESHOLD_Y = 1000; // Right: Y-axis range was 1434  
const int RATTLE_THRESHOLD_X = 1500; // Rattle: X-axis range was 2259

// Detection window variables
struct MovementWindow {
  int minX, maxX, minY, maxY, minZ, maxZ;
  unsigned long startTime;
  bool active;
  int lastX, lastY, lastZ;
};

MovementWindow window = {0, 0, 0, 0, 0, 0, 0, false, 0, 0, 0};

const unsigned long DETECTION_WINDOW = 1000; // 1 second window
const int MOVEMENT_START_THRESHOLD = 100;    // Minimum change to start detection

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
      int rangeY = window.maxY - window.minY;  // FIXED: Use absolute range
      int rangeZ = window.maxZ - window.minZ;
      
      // FIXED: Also check direction for left/right distinction
      int yDirection = (window.maxY + window.minY) / 2; // Average Y position
      
      Serial.printf("🔍 Analysis: X-range=%d, Y-range=%d, Z-range=%d, Y-direction=%d\n", 
                    rangeX, rangeY, rangeZ, yDirection);
      
      String result = "NONE";
      
      // FIXED: Check rattle first (highest priority)
      if (rangeX > RATTLE_THRESHOLD_X) {
        result = "RATTLE";
        Serial.println("🐍 RATTLE DETECTED! (X-axis dominant)");
      }
      // FIXED: Better left/right detection logic
      else if (rangeY > LEFT_THRESHOLD_Y) {
        // Look at the Y-axis data from your captures:
        // LEFT:  Y went from -881 to 286 (range 1167) - negative to positive
        // RIGHT: Y went from 177 to 1611 (range 1434) - positive to higher positive
        
        if (window.minY < -400) { // Left movement goes into negative Y territory
          result = "LEFT";
          Serial.printf("⬅️ LEFT MOVEMENT DETECTED! (Y-range=%d, min=%d)\n", rangeY, window.minY);
        } else if (window.maxY > 800) { // Right movement stays in positive Y territory
          result = "RIGHT";
          Serial.printf("➡️ RIGHT MOVEMENT DETECTED! (Y-range=%d, max=%d)\n", rangeY, window.maxY);
        } else {
          Serial.printf("❓ Y-movement detected but unclear direction (min=%d, max=%d)\n", 
                       window.minY, window.maxY);
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
  Serial.println("🚀   COMPASS CONTROL - FIXED LEFT       🚀");
  Serial.println("🧭========================================🧭");
  Serial.printf("📅 Date: 2025-06-17 12:27:43 UTC\n");
  Serial.printf("👨‍💻 Developer: AmrEslim\n");
  Serial.printf("🔧 Version: Fixed Left Detection\n");
  Serial.println("🧭========================================🧭");
  
  // Print thresholds based on your data
  Serial.println("🎯 MOVEMENT DETECTION LOGIC:");
  Serial.printf("🎯 LEFT:   Y-range > %d AND Y goes negative (< -400)\n", LEFT_THRESHOLD_Y);
  Serial.printf("🎯 RIGHT:  Y-range > %d AND Y stays positive (> 800)\n", LEFT_THRESHOLD_Y);
  Serial.printf("🎯 RATTLE: X-range > %d (highest priority)\n", RATTLE_THRESHOLD_X);
  Serial.println("🧭========================================🧭");
  
  // Initialize compass
  Wire.begin(21, 22);
  compass.init();
  Serial.println("✅ Compass initialized");
  
  delay(2000);
  calibrateCompass();
  
  Serial.println("🤖 READY! Move the compass:");
  Serial.println("   ⬅️ Tilt LEFT (Y should go negative)");  
  Serial.println("   ➡️ Tilt RIGHT (Y should stay positive)");
  Serial.println("   🐍 SHAKE for rattle attack");
  Serial.println("🧭========================================🧭");
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
  
  // Act on detected movement
  if (movement == "LEFT") {
    Serial.println("🎮 ✅ TRIGGERING LEFT MOVEMENT!");
    // Add your snake left movement code here
    delay(2000); // Prevent rapid re-triggering
  }
  else if (movement == "RIGHT") {
    Serial.println("🎮 ✅ TRIGGERING RIGHT MOVEMENT!");
    // Add your snake right movement code here  
    delay(2000); // Prevent rapid re-triggering
  }
  else if (movement == "RATTLE") {
    Serial.println("🎮 ✅ TRIGGERING RATTLE ATTACK!");
    // Add your snake rattle code here
    delay(3000); // Prevent rapid re-triggering
  }
  
  // Enhanced debug output every 2 seconds
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 2000) {
    Serial.printf("📊 Current: X=%d, Y=%d, Z=%d | Window: %s\n", 
                  calX, calY, calZ, window.active ? "ACTIVE" : "IDLE");
    if (window.active) {
      Serial.printf("📊 Window ranges: X=%d-%d, Y=%d-%d, Z=%d-%d\n",
                    window.minX, window.maxX, window.minY, window.maxY, window.minZ, window.maxZ);
    }
    lastDebug = millis();
  }
  
  delay(50); // 20Hz sampling
}