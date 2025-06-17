#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <esp_now.h>
#include <math.h>

WebServer server(80);

// ESP-NOW data structure
typedef struct struct_message {
  float heading;
  float gyroX;
  float gyroY;
  float gyroZ;
  unsigned long timestamp;
} struct_message;

struct_message receivedData;

// ============================================
// PWM CONFIGURATION 
// ============================================
const int PWM_FREQ = 50;         // 50Hz for servo control
const int PWM_RESOLUTION = 16;   // 16-bit resolution
const int PWM_CHANNEL = 0;       // PWM channel for rattle

// Servo configuration
const int NUM_SERVOS = 10;
const int NUM_HORIZONTAL = 7;
const int servoLayout[NUM_SERVOS] = {1, 0, 1, 1, 0, 1, 1, 0, 1, 1};
const int servoPins[NUM_SERVOS] = {23, 22, 2, 4, 16, 17, 5, 18, 19, 21};

// Enhanced Rattling configuration
const int tailLiftIdx = 7;
const int tailRattle1Idx = 9;

bool rattlingActive = false;
unsigned long rattlingStart = 0;
unsigned long RATTLING_DURATION = 5000;
float RATTLING_FREQ = 80.0;
float RATTLING_AMPLITUDE = 21.0;
float RATTLE_LIFT_ANGLE = 45.0;
const float CURL_AMPLITUDE = 70.0;
bool bodyHasCurled = false;
unsigned long lastRattleUpdate = 0;
const int RATTLE_UPDATE_INTERVAL = 5;

Servo servos[NUM_SERVOS];

// Locomotion parameters
bool locomotionEnabled = false;
bool forwardDirection = true;
int movementMode = 0;
float amplitude = 30.0;
float frequency = 1.0;
float phaseOffset = 60.0;
float verticalAmplitude = 20.0;
float verticalPhaseOffset = 90.0;
float centerPosition = 90.0;
unsigned long lastUpdate = 0;
const int interval = 20;

// Servo test variables
bool testingActive = false;
int testServoIndex = 0;
int testPosition = 0;
unsigned long lastTestMillis = 0;

// ============================================
// FIXED GYROSCOPE VARIABLES WITH STATE TRACKING
// ============================================
float gyroX_offset = 0.0;
float gyroY_offset = 0.0;
float gyroZ_offset = 0.0;

// More sensitive thresholds for compass-calculated gyro
float GYRO_THRESHOLD_X = 3.0;   // Reduced further for better sensitivity
float GYRO_THRESHOLD_Y = 5.0;   // Reduced further for better sensitivity  
float GYRO_THRESHOLD_Z = 170.0;  // Reduced for easier rattle trigger
unsigned long MIN_MOVE_TIME = 200;  // Reduced for faster response
bool GYRO_CONTROL_ENABLED = false;

// Calibration process variables
bool isCalibrating = false;
int calibrationSamples = 0;
const int MAX_CALIBRATION_SAMPLES = 30; // Faster calibration
float calibrationSumX = 0;
float calibrationSumY = 0;
float calibrationSumZ = 0;

// Motion detection variables with state tracking
unsigned long moveStartTime = 0;
float lastGyroX = 0, lastGyroY = 0, lastGyroZ = 0;
unsigned long lastGyroUpdate = 0;
const unsigned long GYRO_TIMEOUT = 3000;

// ============================================
// STATE TRACKING VARIABLES
// ============================================
String currentMovementState = "IDLE";
String lastDetectedMovement = "NONE";
float currentGyroMagnitude = 0.0;
bool movementActive = false;
unsigned long lastMovementTime = 0;
int movementCounter = 0;

// Movement detection counters for debugging
int forwardDetections = 0;
int backwardDetections = 0;
int leftDetections = 0;
int rightDetections = 0;
int rattleDetections = 0;

// ESP-NOW callback function with enhanced state tracking
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingData, int len) {
  if (len == sizeof(receivedData)) {
    memcpy(&receivedData, incomingData, sizeof(receivedData));
    
    lastGyroUpdate = millis();
    processGyroscopeData(receivedData.gyroX, receivedData.gyroY, receivedData.gyroZ);
    
    // Update current gyro magnitude for state display
    currentGyroMagnitude = sqrt(pow(lastGyroX, 2) + pow(lastGyroY, 2) + pow(lastGyroZ, 2));
  }
}

// ============================================
// IMPROVED GYROSCOPE FUNCTIONS
// ============================================

void startGyroCalibration() {
  Serial.println("🔧 ========================================");
  Serial.println("🔧 STARTING ENHANCED GYROSCOPE CALIBRATION");
  Serial.println("🔧 ========================================");
  
  isCalibrating = true;
  calibrationSamples = 0;
  calibrationSumX = 0;
  calibrationSumY = 0;
  calibrationSumZ = 0;
  
  // Stop any current snake movement during calibration
  locomotionEnabled = false;
  rattlingActive = false;
  testingActive = false;
  currentMovementState = "CALIBRATING";
  centerAllServos();
  
  Serial.printf("📊 Keep device PERFECTLY STILL for %d samples (about 3 seconds)\n", MAX_CALIBRATION_SAMPLES);
  Serial.println("🔧 ========================================");
}

void processCalibrationSample(float gx, float gy, float gz) {
  if (!isCalibrating) return;
  
  calibrationSumX += gx;
  calibrationSumY += gy;
  calibrationSumZ += gz;
  calibrationSamples++;
  
  // Progress feedback
  if (calibrationSamples % 5 == 0) {
    Serial.printf("📊 Calibration: %d/%d samples (%.0f%%) - Current: X:%.2f Y:%.2f Z:%.2f\n", 
                  calibrationSamples, MAX_CALIBRATION_SAMPLES, 
                  (calibrationSamples * 100.0) / MAX_CALIBRATION_SAMPLES,
                  gx, gy, gz);
  }
  
  if (calibrationSamples >= MAX_CALIBRATION_SAMPLES) {
    // Calculate offsets
    gyroX_offset = calibrationSumX / calibrationSamples;
    gyroY_offset = calibrationSumY / calibrationSamples;
    gyroZ_offset = calibrationSumZ / calibrationSamples;
    
    isCalibrating = false;
    currentMovementState = "IDLE";
    
    Serial.println("✅ ========================================");
    Serial.println("✅ GYROSCOPE CALIBRATION COMPLETE!");
    Serial.printf("📊 Calculated offsets: X:%.3f Y:%.3f Z:%.3f\n", 
                  gyroX_offset, gyroY_offset, gyroZ_offset);
    Serial.printf("🎯 Active thresholds: X:%.1f Y:%.1f Z:%.1f°/s\n",
                  GYRO_THRESHOLD_X, GYRO_THRESHOLD_Y, GYRO_THRESHOLD_Z);
    Serial.println("✅ ========================================");
  }
}

void processGyroscopeData(float gx, float gy, float gz) {
  // If calibrating, add to calibration samples
  if (isCalibrating) {
    processCalibrationSample(gx, gy, gz);
    return;
  }
  
  // Apply calibration offsets
  float calibratedX = gx - gyroX_offset;
  float calibratedY = gy - gyroY_offset;
  float calibratedZ = gz - gyroZ_offset;
  
  // Store for web UI display
  lastGyroX = calibratedX;
  lastGyroY = calibratedY;
  lastGyroZ = calibratedZ;
  
  // Always log the values for debugging (every 20 samples to avoid spam)
  static int debugCounter = 0;
  if (debugCounter++ % 20 == 0) {
    Serial.printf("🎯 Gyro Debug: X:%.2f Y:%.2f Z:%.2f | Thresholds: X:%.1f Y:%.1f Z:%.1f | Control:%s\n",
                  calibratedX, calibratedY, calibratedZ,
                  GYRO_THRESHOLD_X, GYRO_THRESHOLD_Y, GYRO_THRESHOLD_Z,
                  GYRO_CONTROL_ENABLED ? "ON" : "OFF");
  }
  
  // Check if gyro control is enabled
  if (!GYRO_CONTROL_ENABLED) {
    currentMovementState = "GYRO_DISABLED";
    return;
  }
  
  // FIXED: Better movement detection logic
  bool xActive = fabs(calibratedX) > GYRO_THRESHOLD_X;
  bool yActive = fabs(calibratedY) > GYRO_THRESHOLD_Y;
  bool zActive = fabs(calibratedZ) > GYRO_THRESHOLD_Z;
  
  bool anyMovement = xActive || yActive || zActive;
  
  if (anyMovement) {
    if (moveStartTime == 0) {
      moveStartTime = millis();
      
      // FIXED: Determine movement based on STRONGEST axis, not just any active axis
      float absX = fabs(calibratedX);
      float absY = fabs(calibratedY);
      float absZ = fabs(calibratedZ);
      
      String detectedMovement = "UNKNOWN";
      
      // FIXED: Only trigger rattle if Z is DOMINANT and above threshold
      if (zActive && absZ > absX && absZ > absY && absZ > (GYRO_THRESHOLD_Z + 2.0)) {
        detectedMovement = "RATTLE";
        rattleDetections++;
        Serial.printf("🐍 RATTLE DETECTED: Z=%.2f (threshold=%.1f, dominant over X=%.2f Y=%.2f)\n", 
                      absZ, GYRO_THRESHOLD_Z, absX, absY);
      } 
      // FIXED: Better axis priority logic
      else if (yActive && absY > absX && absY > absZ) {
        if (calibratedY > 0) {
          detectedMovement = "FORWARD";
          forwardDetections++;
          Serial.printf("⬆️ FORWARD DETECTED: Y=%.2f (positive, dominant)\n", calibratedY);
        } else {
          detectedMovement = "LEFT";
          leftDetections++;
          Serial.printf("⬅️ LEFT DETECTED: Y=%.2f (negative, dominant)\n", calibratedY);
        }
      } 
      else if (xActive && absX > absY && absX > absZ) {
        if (calibratedX > 0) {
          detectedMovement = "RIGHT";
          rightDetections++;
          Serial.printf("➡️ RIGHT DETECTED: X=%.2f (positive, dominant)\n", calibratedX);
        } else {
          detectedMovement = "BACKWARD";
          backwardDetections++;
          Serial.printf("⬇️ BACKWARD DETECTED: X=%.2f (negative, dominant)\n", calibratedX);
        }
      }
      // FIXED: If movement detected but not dominant enough, ignore
      else {
        Serial.printf("🔍 Movement detected but not dominant enough - X:%.2f Y:%.2f Z:%.2f\n", 
                      absX, absY, absZ);
        moveStartTime = 0; // Reset immediately
        return;
      }
      
      lastDetectedMovement = detectedMovement;
      currentMovementState = "DETECTING_" + detectedMovement;
    }
    
    // Check if movement has been sustained long enough
    if (millis() - moveStartTime >= MIN_MOVE_TIME) {
      triggerMovement(lastDetectedMovement, calibratedX, calibratedY, calibratedZ);
      movementActive = true;
      lastMovementTime = millis();
      movementCounter++;
    }
  } else {
    // Reset movement detection
    if (moveStartTime != 0) {
      Serial.printf("🔄 Movement ended after %lu ms (not sustained enough)\n", millis() - moveStartTime);
    }
    moveStartTime = 0;
    movementActive = false;
    if (!locomotionEnabled && !rattlingActive && !testingActive) {
      currentMovementState = "IDLE";
    }
  }
}

void setupRattlePWM() {
  Serial.println("🐍 Setting up enhanced PWM for high-speed rattling...");
  
  // Use new ESP32 Core 3.x LEDC functions
  ledcAttach(servoPins[tailRattle1Idx], PWM_FREQ, PWM_RESOLUTION);
  
  Serial.printf("✅ PWM configured: Pin %d, Freq %dHz, Resolution %d-bit\n", 
                servoPins[tailRattle1Idx], PWM_FREQ, PWM_RESOLUTION);
}
void triggerMovement(String movement, float gx, float gy, float gz) {
  Serial.println("🚀 ========================================");
  Serial.printf("🚀 TRIGGERING MOVEMENT: %s\n", movement.c_str());
  Serial.printf("🚀 Gyro values: X:%.2f Y:%.2f Z:%.2f\n", gx, gy, gz);
  
  // Stop current movement first
  locomotionEnabled = false;
  
  if (movement == "RATTLE") {
    Serial.println("🐍 >>> ACTIVATING RATTLE ATTACK!");
    currentMovementState = "RATTLING";
    rattleTail();
  } else if (movement == "FORWARD") {
    Serial.println("⬆️ >>> ACTIVATING FORWARD MOTION");
    currentMovementState = "MOVING_FORWARD";
    movementMode = 0; // Lateral undulation
    forwardDirection = true;
    locomotionEnabled = true;
  } else if (movement == "BACKWARD") {
    Serial.println("⬇️ >>> ACTIVATING BACKWARD MOTION");
    currentMovementState = "MOVING_BACKWARD";
    movementMode = 0; // Lateral undulation
    forwardDirection = false;
    locomotionEnabled = true;
  } else if (movement == "LEFT") {
    Serial.println("⬅️ >>> ACTIVATING LEFT SIDEWINDING");
    currentMovementState = "MOVING_LEFT";
    movementMode = 1; // Sidewinding
    forwardDirection = false;
    locomotionEnabled = true;
  } else if (movement == "RIGHT") {
    Serial.println("➡️ >>> ACTIVATING RIGHT SIDEWINDING");
    currentMovementState = "MOVING_RIGHT";
    movementMode = 1; // Sidewinding
    forwardDirection = true;
    locomotionEnabled = true;
  }
  
  Serial.println("🚀 ========================================");
}

// Enhanced HTML with comprehensive state display
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>🐍 Snake Robot Control v2.3 - State Monitor</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      padding: 20px;
      color: #333;
    }
    .container {
      max-width: 1200px;
      margin: 0 auto;
    }
    .header {
      text-align: center;
      color: white;
      margin-bottom: 30px;
    }
    .header h1 {
      font-size: 2.8rem;
      margin-bottom: 10px;
      text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
      animation: pulse 3s infinite;
    }
    @keyframes pulse {
      0%, 100% { transform: scale(1); }
      50% { transform: scale(1.02); }
    }
    .header .subtitle {
      font-size: 1.1rem;
      opacity: 0.9;
      margin-bottom: 5px;
    }
    .header .timestamp {
      font-size: 0.9rem;
      opacity: 0.7;
    }
    
    /* STATE MONITOR STYLES */
    .state-monitor {
      background: linear-gradient(135deg, #1e293b, #334155);
      border: 3px solid #0ea5e9;
      border-radius: 20px;
      padding: 25px;
      margin-bottom: 25px;
      color: white;
      box-shadow: 0 20px 40px rgba(0,0,0,0.3);
    }
    .state-title {
      font-size: 1.5rem;
      font-weight: bold;
      margin-bottom: 20px;
      text-align: center;
      color: #0ea5e9;
      text-shadow: 0 0 10px rgba(14, 165, 233, 0.5);
    }
    .state-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 15px;
      margin-bottom: 20px;
    }
    .state-item {
      background: rgba(255,255,255,0.1);
      border: 1px solid rgba(255,255,255,0.2);
      border-radius: 12px;
      padding: 15px;
      text-align: center;
      backdrop-filter: blur(10px);
    }
    .state-label {
      font-size: 0.9rem;
      opacity: 0.8;
      margin-bottom: 5px;
    }
    .state-value {
      font-size: 1.2rem;
      font-weight: bold;
      color: #fbbf24;
    }
    .state-value.active {
      color: #22c55e;
      animation: glow 2s infinite;
    }
    .state-value.alert {
      color: #ef4444;
      animation: blink 1s infinite;
    }
    @keyframes glow {
      0%, 100% { text-shadow: 0 0 5px currentColor; }
      50% { text-shadow: 0 0 20px currentColor; }
    }
    @keyframes blink {
      0%, 50% { opacity: 1; }
      51%, 100% { opacity: 0.3; }
    }
    
    .detection-counters {
      display: grid;
      grid-template-columns: repeat(5, 1fr);
      gap: 10px;
      margin-top: 15px;
    }
    .counter-item {
      background: rgba(59, 130, 246, 0.2);
      border: 1px solid rgba(59, 130, 246, 0.4);
      border-radius: 8px;
      padding: 10px;
      text-align: center;
    }
    .counter-label {
      font-size: 0.8rem;
      opacity: 0.8;
    }
    .counter-value {
      font-size: 1.1rem;
      font-weight: bold;
      color: #60a5fa;
    }
    
    .card {
      background: white;
      border-radius: 18px;
      padding: 25px;
      margin-bottom: 20px;
      box-shadow: 0 15px 35px rgba(0,0,0,0.1);
      backdrop-filter: blur(10px);
      border: 1px solid rgba(255,255,255,0.2);
      transition: all 0.3s ease;
    }
    .card:hover {
      transform: translateY(-5px);
      box-shadow: 0 20px 40px rgba(0,0,0,0.15);
    }
    .card-title {
      font-size: 1.4rem;
      font-weight: bold;
      margin-bottom: 20px;
      color: #333;
      display: flex;
      align-items: center;
      gap: 10px;
    }
    .control-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 15px;
      margin-bottom: 20px;
    }
    .param-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 20px;
    }
    .btn {
      padding: 15px 20px;
      border: none;
      border-radius: 12px;
      font-size: 1rem;
      font-weight: bold;
      cursor: pointer;
      transition: all 0.3s ease;
      text-transform: uppercase;
      position: relative;
      overflow: hidden;
    }
    .btn::before {
      content: '';
      position: absolute;
      top: 0;
      left: -100%;
      width: 100%;
      height: 100%;
      background: linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent);
      transition: left 0.5s;
    }
    .btn:hover::before {
      left: 100%;
    }
    .btn:hover {
      transform: translateY(-3px);
      box-shadow: 0 8px 25px rgba(0,0,0,0.3);
    }
    .btn-primary { background: linear-gradient(135deg, #3b82f6, #1d4ed8); color: white; }
    .btn-success { background: linear-gradient(135deg, #10b981, #059669); color: white; }
    .btn-danger { background: linear-gradient(135deg, #ef4444, #dc2626); color: white; }
    .btn-warning { background: linear-gradient(135deg, #f59e0b, #d97706); color: white; }
    .btn-info { background: linear-gradient(135deg, #06b6d4, #0891b2); color: white; }
    .btn-rattle {
      background: linear-gradient(45deg, #8b5cf6, #a78bfa, #c084fc);
      color: white;
      font-size: 1.1rem;
      animation: rattleGlow 2s infinite;
      border: 2px solid #7c3aed;
      font-weight: 900;
      letter-spacing: 1px;
    }
    .btn-gyro {
      background: linear-gradient(45deg, #f59e0b, #fbbf24, #fcd34d);
      color: white;
      font-size: 1.1rem;
      border: 2px solid #d97706;
      font-weight: 900;
    }
    .btn-gyro.active {
      background: linear-gradient(45deg, #10b981, #34d399, #6ee7b7);
      animation: gyroGlow 2s infinite;
    }
    @keyframes rattleGlow {
      0%, 100% { 
        box-shadow: 0 0 20px rgba(139, 92, 246, 0.6),
                    0 0 40px rgba(139, 92, 246, 0.3),
                    inset 0 0 10px rgba(255,255,255,0.2);
      }
      50% { 
        box-shadow: 0 0 30px rgba(139, 92, 246, 0.8),
                    0 0 60px rgba(139, 92, 246, 0.5),
                    inset 0 0 15px rgba(255,255,255,0.3);
      }
    }
    @keyframes gyroGlow {
      0%, 100% { 
        box-shadow: 0 0 20px rgba(16, 185, 129, 0.6),
                    0 0 40px rgba(16, 185, 129, 0.3),
                    inset 0 0 10px rgba(255,255,255,0.2);
      }
      50% { 
        box-shadow: 0 0 30px rgba(16, 185, 129, 0.8),
                    0 0 60px rgba(16, 185, 129, 0.5),
                    inset 0 0 15px rgba(255,255,255,0.3);
      }
    }
    select, input {
      width: 100%;
      padding: 12px 16px;
      border: 2px solid #e5e7eb;
      border-radius: 10px;
      font-size: 1rem;
      margin-bottom: 10px;
      transition: all 0.3s ease;
      background: white;
    }
    select:focus, input:focus {
      outline: none;
      border-color: #3b82f6;
      box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
    }
    .param-group {
      margin-bottom: 15px;
    }
    .param-label {
      display: block;
      margin-bottom: 8px;
      font-weight: bold;
      color: #374151;
      font-size: 0.95rem;
    }
    .param-description {
      font-size: 0.8rem;
      color: #6b7280;
      margin-bottom: 5px;
    }
    .status {
      background: linear-gradient(135deg, #e0f2fe, #f0f9ff);
      border: 2px solid #0ea5e9;
      border-radius: 15px;
      padding: 20px;
      text-align: center;
      margin-top: 20px;
      position: relative;
      overflow: hidden;
    }
    .status::before {
      content: '';
      position: absolute;
      top: 0;
      left: -100%;
      width: 100%;
      height: 100%;
      background: linear-gradient(90deg, transparent, rgba(14, 165, 233, 0.1), transparent);
      animation: statusSweep 3s infinite;
    }
    @keyframes statusSweep {
      0% { left: -100%; }
      100% { left: 100%; }
    }
    .status-text {
      font-size: 1.3rem;
      font-weight: bold;
      color: #0369a1;
      position: relative;
      z-index: 1;
    }
    .gyro-status {
      background: linear-gradient(135deg, #fef3c7, #fde68a);
      border: 2px solid #f59e0b;
      border-radius: 15px;
      padding: 15px;
      margin-bottom: 15px;
    }
    .gyro-status.active {
      background: linear-gradient(135deg, #dcfce7, #f0fdf4);
      border: 2px solid #22c55e;
    }
    .gyro-data {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 10px;
      margin-top: 10px;
    }
    .gyro-axis {
      text-align: center;
      padding: 10px;
      background: white;
      border-radius: 8px;
      font-weight: bold;
    }
    .notification {
      position: fixed;
      top: 20px;
      right: 20px;
      background: #10b981;
      color: white;
      padding: 15px 25px;
      border-radius: 12px;
      font-weight: bold;
      transform: translateX(400px);
      transition: transform 0.4s cubic-bezier(0.175, 0.885, 0.32, 1.275);
      z-index: 1000;
      max-width: 350px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.3);
    }
    .notification.show {
      transform: translateX(0);
    }
    .notification.error {
      background: #ef4444;
    }
    .gyro-controls {
      background: linear-gradient(135deg, #dbeafe, #e0f2fe);
      border: 2px solid #0ea5e9;
      border-radius: 15px;
      padding: 20px;
      margin-top: 15px;
    }
    .gyro-title {
      color: #0369a1;
      font-weight: bold;
      margin-bottom: 15px;
      font-size: 1.1rem;
    }
    .rattle-controls {
      background: linear-gradient(135deg, #fef3c7, #fde68a);
      border: 2px solid #f59e0b;
      border-radius: 15px;
      padding: 20px;
      margin-top: 15px;
    }
    .rattle-title {
      color: #92400e;
      font-weight: bold;
      margin-bottom: 15px;
      font-size: 1.1rem;
    }
    .slider-container {
      margin: 15px 0;
    }
    .slider {
      width: 100%;
      height: 8px;
      border-radius: 5px;
      background: #d1d5db;
      outline: none;
      opacity: 0.7;
      transition: opacity 0.2s;
      -webkit-appearance: none;
    }
    .slider:hover {
      opacity: 1;
    }
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: #8b5cf6;
      cursor: pointer;
      box-shadow: 0 0 10px rgba(139, 92, 246, 0.5);
    }
    .slider::-moz-range-thumb {
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: #8b5cf6;
      cursor: pointer;
      border: none;
      box-shadow: 0 0 10px rgba(139, 92, 246, 0.5);
    }
    .value-display {
      font-weight: bold;
      color: #7c2d12;
      float: right;
    }
    .footer {
      text-align: center;
      color: white;
      margin-top: 30px;
      opacity: 0.8;
    }
    /* Mobile Responsiveness */
    @media (max-width: 768px) {
      .header h1 { font-size: 2.2rem; }
      .control-grid { grid-template-columns: repeat(2, 1fr); }
      .param-grid { grid-template-columns: 1fr; }
      .btn-rattle { grid-column: 1 / -1; }
      .state-grid { grid-template-columns: 1fr; }
      .detection-counters { grid-template-columns: repeat(3, 1fr); }
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>🐍 SNAKE ROBOT CONTROL 🐍</h1>
      <div class="subtitle">Enhanced Bio-Inspired Locomotion with Real-Time State Monitoring</div>
      <div class="timestamp">Updated: 2025-06-17 10:39:04 UTC | State Monitor v2.3 | By AmrEslim</div>
    </div>

    <!-- STATE MONITOR SECTION -->
    <div class="state-monitor">
      <div class="state-title">🔍 REAL-TIME STATE MONITOR</div>
      
      <div class="state-grid">
        <div class="state-item">
          <div class="state-label">Current State</div>
          <div class="state-value active" id="currentState">IDLE</div>
        </div>
        <div class="state-item">
          <div class="state-label">Last Movement</div>
          <div class="state-value" id="lastMovement">NONE</div>
        </div>
        <div class="state-item">
          <div class="state-label">Gyro Magnitude</div>
          <div class="state-value" id="gyroMagnitude">0.00°/s</div>
        </div>
        <div class="state-item">
          <div class="state-label">Movement Counter</div>
          <div class="state-value" id="movementCounter">0</div>
        </div>
        <div class="state-item">
          <div class="state-label">Gyro Control</div>
          <div class="state-value" id="gyroControlState">DISABLED</div>
        </div>
        <div class="state-item">
          <div class="state-label">Data Connection</div>
          <div class="state-value" id="dataConnection">WAITING</div>
        </div>
      </div>
      
      <div class="detection-counters">
        <div class="counter-item">
          <div class="counter-label">Forward</div>
          <div class="counter-value" id="forwardCount">0</div>
        </div>
        <div class="counter-item">
          <div class="counter-label">Backward</div>
          <div class="counter-value" id="backwardCount">0</div>
        </div>
        <div class="counter-item">
          <div class="counter-label">Left</div>
          <div class="counter-value" id="leftCount">0</div>
        </div>
        <div class="counter-item">
          <div class="counter-label">Right</div>
          <div class="counter-value" id="rightCount">0</div>
        </div>
        <div class="counter-item">
          <div class="counter-label">Rattle</div>
          <div class="counter-value" id="rattleCount">0</div>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">🎯 Gyroscope Control System</div>
      <div class="gyro-status" id="gyroStatus">
        <div><strong>🔧 Calibration Status:</strong> <span id="calibrationStatus">Ready to Calibrate</span></div>
        <div><strong>📡 Data Status:</strong> <span id="dataStatus">Waiting for Connection...</span></div>
        <div><strong>🎮 Control Status:</strong> <span id="controlStatus">Disabled</span></div>
      </div>
      
      <div class="gyro-data">
        <div class="gyro-axis">
          <div>Gyro X</div>
          <div id="gyroX">0.00°/s</div>
        </div>
        <div class="gyro-axis">
          <div>Gyro Y</div>
          <div id="gyroY">0.00°/s</div>
        </div>
        <div class="gyro-axis">
          <div>Gyro Z</div>
          <div id="gyroZ">0.00°/s</div>
        </div>
      </div>
      
      <div class="control-grid">
        <button class="btn btn-info" id="calibrateBtn">🔧 Calibrate Gyro</button>
        <button class="btn btn-gyro" id="gyroControlBtn">🎮 Enable Gyro Control</button>
        <button class="btn btn-warning" id="gyroSettingsBtn">⚙️ Update Gyro Settings</button>
      </div>
    </div>

    <div class="card">
      <div class="card-title">🎯 Movement Mode</div>
      <select id="modeSelect">
        <option value="0">🌊 Lateral Undulation (Snake-like)</option>
        <option value="1">🏜️ Sidewinding (Desert Movement)</option>
      </select>
    </div>

    <div class="card">
      <div class="card-title">🎮 Robot Controls</div>
      <div class="control-grid">
        <button class="btn btn-primary" id="forwardBtn">⬆️ Forward</button>
        <button class="btn btn-primary" id="backwardBtn">⬇️ Backward</button>
        <button class="btn btn-danger" id="stopBtn">⏹️ Stop</button>
        <button class="btn btn-success" id="centerBtn">🎯 Center</button>
        <button class="btn btn-warning" id="testBtn">🔧 Test</button>
        <button class="btn btn-rattle" id="rattleBtn">🐍 RATTLE ATTACK 🐍</button>
      </div>
    </div>

    <div class="card">
      <div class="card-title">⚙️ Locomotion Parameters</div>
      <div class="param-grid">
        <div class="param-group">
          <label class="param-label">📏 Movement Amplitude</label>
          <div class="param-description">Controls how wide the snake moves (10-45°)</div>
          <input type="number" id="amplitude" min="10" max="45" step="1" value="30">
        </div>
        <div class="param-group">
          <label class="param-label">⚡ Movement Frequency</label>
          <div class="param-description">Controls movement speed (0.1-2.0 Hz)</div>
          <input type="number" id="frequency" min="0.1" max="2.0" step="0.1" value="1.0">
        </div>
        <div class="param-group">
          <label class="param-label">🌊 Phase Offset</label>
          <div class="param-description">Controls wave propagation (30-90°)</div>
          <input type="number" id="phase" min="30" max="90" step="5" value="60">
        </div>
      </div>
      <button class="btn btn-success" id="updateBtn">💾 Update Locomotion</button>
      
      <div class="gyro-controls">
        <div class="gyro-title">🎯 ENHANCED GYROSCOPE SETTINGS</div>
        <div class="param-grid">
          <div class="param-group">
            <label class="param-label">🔄 X-Axis Threshold (Pitch)</label>
            <div class="param-description">Forward/Backward sensitivity: 3.0°/s (Range: 1-200 °/s)</div>
            <div class="slider-container">
              <input type="range" id="gyroThresholdX" class="slider" min="1" max="200" step="0.5" value="3">
              <span class="value-display" id="gyroThresholdXValue">3.0 °/s</span>
            </div>
          </div>
          <div class="param-group">
            <label class="param-label">🔄 Y-Axis Threshold (Roll)</label>
            <div class="param-description">Left/Right sensitivity: 5.0°/s (Range: 1-200 °/s)</div>
            <div class="slider-container">
              <input type="range" id="gyroThresholdY" class="slider" min="1" max="200" step="0.5" value="5">
              <span class="value-display" id="gyroThresholdYValue">5.0 °/s</span>
            </div>
          </div>
          <div class="param-group">
            <label class="param-label">🔄 Z-Axis Threshold (Yaw)</label>
            <div class="param-description">Rattle sensitivity: 107.0°/s (Range: 5-400 °/s)</div>
            <div class="slider-container">
              <input type="range" id="gyroThresholdZ" class="slider" min="5" max="400" step="1" value="170">
              <span class="value-display" id="gyroThresholdZValue">170.0 °/s</span>
            </div>
          </div>
          <div class="param-group">
            <label class="param-label">⏱️ Motion Duration</label>
            <div class="param-description">Minimum motion time: 200ms (Range: 50-1000 ms)</div>
            <div class="slider-container">
              <input type="range" id="minMoveTime" class="slider" min="50" max="1000" step="50" value="200">
              <span class="value-display" id="minMoveTimeValue">200 ms</span>
            </div>
          </div>
        </div>
      </div>
      
      <div class="rattle-controls">
        <div class="rattle-title">🐍 RATTLE CONFIGURATION 🐍</div>
        <div class="param-grid">
          <div class="param-group">
            <label class="param-label">⚡ Rattle Frequency</label>
            <div class="param-description">How fast the tail vibrates (10-80 Hz)</div>
            <div class="slider-container">
              <input type="range" id="rattleFreq" class="slider" min="10" max="80" step="5" value="80">
              <span class="value-display" id="rattleFreqValue">80 Hz</span>
            </div>
          </div>
          <div class="param-group">
            <label class="param-label">📐 Rattle Amplitude</label>
            <div class="param-description">How wide the tail swings (5-50°)</div>
            <div class="slider-container">
              <input type="range" id="rattleAmp" class="slider" min="5" max="50" step="2" value="21">
              <span class="value-display" id="rattleAmpValue">21°</span>
            </div>
          </div>
          <div class="param-group">
            <label class="param-label">⬆️ Tail Lift Angle</label>
            <div class="param-description">How high to lift the tail (20-70°)</div>
            <div class="slider-container">
              <input type="range" id="rattleLift" class="slider" min="20" max="70" step="5" value="45">
              <span class="value-display" id="rattleLiftValue">45°</span>
            </div>
          </div>
          <div class="param-group">
            <label class="param-label">⏱️ Rattle Duration</label>
            <div class="param-description">How long to rattle (2-10 seconds)</div>
            <div class="slider-container">
              <input type="range" id="rattleDuration" class="slider" min="2" max="10" step="1" value="5">
              <span class="value-display" id="rattleDurationValue">5 sec</span>
            </div>
          </div>
        </div>
        <button class="btn btn-warning" id="updateRattleBtn">🔧 Apply Rattle Settings</button>
      </div>
    </div>

    <div class="status">
      <div class="status-text">🤖 Status: <span id="statusText">Ready for Commands</span></div>
    </div>

    <div class="footer">
      <p>🚀 Snake Robot v2.3 State Monitor Edition | Developed by AmrEslim | 2025 🚀</p>
      <p>🐍 Enhanced Bio-Inspired Robotics with Real-Time Diagnostics 🐍</p>
    </div>
  </div>

  <div class="notification" id="notification"></div>

  <script>
    // Global state variables
    var currentStatus = 'Ready for Commands';
    var gyroControlActive = false;
    var lastGyroUpdate = 0;
    var stateUpdateInterval;
    
    // State tracking
    var currentState = 'IDLE';
    var lastMovement = 'NONE';
    var gyroMagnitude = 0;
    var movementCounter = 0;
    var detectionCounts = {
      forward: 0,
      backward: 0,
      left: 0,
      right: 0,
      rattle: 0
    };
    
    // DOM elements
    var statusElement = document.getElementById('statusText');
    var notificationElement = document.getElementById('notification');
    var modeSelect = document.getElementById('modeSelect');
    var amplitudeInput = document.getElementById('amplitude');
    var frequencyInput = document.getElementById('frequency');
    var phaseInput = document.getElementById('phase');
    
    // State monitor elements
    var currentStateElement = document.getElementById('currentState');
    var lastMovementElement = document.getElementById('lastMovement');
    var gyroMagnitudeElement = document.getElementById('gyroMagnitude');
    var movementCounterElement = document.getElementById('movementCounter');
    var gyroControlStateElement = document.getElementById('gyroControlState');
    var dataConnectionElement = document.getElementById('dataConnection');
    
    // Detection counter elements
    var forwardCountElement = document.getElementById('forwardCount');
    var backwardCountElement = document.getElementById('backwardCount');
    var leftCountElement = document.getElementById('leftCount');
    var rightCountElement = document.getElementById('rightCount');
    var rattleCountElement = document.getElementById('rattleCount');
    
    // Gyro control elements
    var calibrateBtn = document.getElementById('calibrateBtn');
    var gyroControlBtn = document.getElementById('gyroControlBtn');
    var gyroSettingsBtn = document.getElementById('gyroSettingsBtn');
    var calibrationStatus = document.getElementById('calibrationStatus');
    var dataStatus = document.getElementById('dataStatus');
    var controlStatus = document.getElementById('controlStatus');
    var gyroXElement = document.getElementById('gyroX');
    var gyroYElement = document.getElementById('gyroY');
    var gyroZElement = document.getElementById('gyroZ');
    
    // Gyro threshold elements
    var gyroThresholdXSlider = document.getElementById('gyroThresholdX');
    var gyroThresholdYSlider = document.getElementById('gyroThresholdY');
    var gyroThresholdZSlider = document.getElementById('gyroThresholdZ');
    var minMoveTimeSlider = document.getElementById('minMoveTime');
    
    // Rattle control elements
    var rattleFreqSlider = document.getElementById('rattleFreq');
    var rattleAmpSlider = document.getElementById('rattleAmp');
    var rattleLiftSlider = document.getElementById('rattleLift');
    var rattleDurationSlider = document.getElementById('rattleDuration');
    
    // Status update function
    function updateStatus(status) {
      currentStatus = status;
      if (statusElement) {
        statusElement.textContent = status;
      }
      console.log('Status: ' + status);
    }
    
    // State update function
    function updateStateDisplay() {
      if (currentStateElement) currentStateElement.textContent = currentState;
      if (lastMovementElement) lastMovementElement.textContent = lastMovement;
      if (gyroMagnitudeElement) gyroMagnitudeElement.textContent = gyroMagnitude.toFixed(2) + '°/s';
      if (movementCounterElement) movementCounterElement.textContent = movementCounter;
      if (gyroControlStateElement) {
        gyroControlStateElement.textContent = gyroControlActive ? 'ENABLED' : 'DISABLED';
        gyroControlStateElement.className = gyroControlActive ? 'state-value active' : 'state-value alert';
      }
      
      // Update detection counters
      if (forwardCountElement) forwardCountElement.textContent = detectionCounts.forward;
      if (backwardCountElement) backwardCountElement.textContent = detectionCounts.backward;
      if (leftCountElement) leftCountElement.textContent = detectionCounts.left;
      if (rightCountElement) rightCountElement.textContent = detectionCounts.right;
      if (rattleCountElement) rattleCountElement.textContent = detectionCounts.rattle;
    }
    
    // Notification function
    function showNotification(message, isError) {
      if (!notificationElement) return;
      
      notificationElement.textContent = message;
      notificationElement.className = 'notification show' + (isError ? ' error' : '');
      
      setTimeout(function() {
        notificationElement.classList.remove('show');
      }, 5000);
    }
    
    // HTTP request function
    function makeRequest(data) {
      console.log('Making request: ' + data);
      updateStatus('🔄 Processing...');
      
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/', true);
      xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
      
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) {
            console.log('Response: ' + xhr.responseText);
            showNotification(xhr.responseText, false);
            
            // Parse response for state updates
            parseStateResponse(xhr.responseText);
          } else {
            console.error('Request failed: ' + xhr.status);
            showNotification('❌ Error: Request failed', true);
            updateStatus('❌ Error');
          }
        }
      };
      
      xhr.onerror = function() {
        console.error('Network error');
        showNotification('🌐 Network error', true);
        updateStatus('🌐 Disconnected');
      };
      
      xhr.send(data);
    }
    
    // Enhanced response parser for state data
    function parseStateResponse(response) {
      // Update gyro status based on response
      if (response.includes('STATE DATA')) {
        var parts = response.split('|');
        for (var i = 0; i < parts.length; i++) {
          var part = parts[i].trim();
          if (part.startsWith('State:')) {
            currentState = part.split(':')[1].trim();
          } else if (part.startsWith('LastMove:')) {
            lastMovement = part.split(':')[1].trim();
          } else if (part.startsWith('Counter:')) {
            movementCounter = parseInt(part.split(':')[1]) || 0;
          } else if (part.startsWith('GyroX:')) {
            var value = parseFloat(part.split(':')[1]);
            if (gyroXElement) gyroXElement.textContent = value.toFixed(2) + '°/s';
          } else if (part.startsWith('GyroY:')) {
            var value = parseFloat(part.split(':')[1]);
            if (gyroYElement) gyroYElement.textContent = value.toFixed(2) + '°/s';
          } else if (part.startsWith('GyroZ:')) {
            var value = parseFloat(part.split(':')[1]);
            if (gyroZElement) gyroZElement.textContent = value.toFixed(2) + '°/s';
          } else if (part.startsWith('Magnitude:')) {
            gyroMagnitude = parseFloat(part.split(':')[1]) || 0;
          } else if (part.startsWith('F:')) {
            detectionCounts.forward = parseInt(part.split(':')[1]) || 0;
          } else if (part.startsWith('B:')) {
            detectionCounts.backward = parseInt(part.split(':')[1]) || 0;
          } else if (part.startsWith('L:')) {
            detectionCounts.left = parseInt(part.split(':')[1]) || 0;
          } else if (part.startsWith('R:')) {
            detectionCounts.right = parseInt(part.split(':')[1]) || 0;
          } else if (part.startsWith('Z:')) {
            detectionCounts.rattle = parseInt(part.split(':')[1]) || 0;
          }
        }
        
        if (dataStatus) dataStatus.textContent = 'Connected';
        if (dataConnectionElement) {
          dataConnectionElement.textContent = 'CONNECTED';
          dataConnectionElement.className = 'state-value active';
        }
        document.getElementById('gyroStatus').className = 'gyro-status active';
        lastGyroUpdate = Date.now();
        
        updateStateDisplay();
      }
      
      if (response.includes('CALIBRATION COMPLETE') || response.includes('calibration complete')) {
        if (calibrationStatus) calibrationStatus.textContent = 'Calibrated';
        currentState = 'CALIBRATED';
        updateStateDisplay();
      }
    }
    
    // Gyro control functions
    function calibrateGyro() {
      currentState = 'CALIBRATING';
      updateStateDisplay();
      updateStatus('🔧 Starting Enhanced Gyroscope Calibration');
      makeRequest('action=calibrateGyro');
    }
    
    function toggleGyroControl() {
      gyroControlActive = !gyroControlActive;
      var action = gyroControlActive ? 'enableGyroControl' : 'disableGyroControl';
      var buttonText = gyroControlActive ? '🎮 Disable Gyro Control' : '🎮 Enable Gyro Control';
      
      gyroControlBtn.textContent = buttonText;
      gyroControlBtn.className = gyroControlActive ? 'btn btn-gyro active' : 'btn btn-gyro';
      
      if (controlStatus) controlStatus.textContent = gyroControlActive ? 'Enabled' : 'Disabled';
      
      currentState = gyroControlActive ? 'GYRO_ENABLED' : 'GYRO_DISABLED';
      updateStateDisplay();
      
      updateStatus(gyroControlActive ? '🎮 Enhanced Gyro Control Enabled' : '🎮 Gyro Control Disabled');
      makeRequest('action=' + action);
    }
    
    function updateGyroSettings() {
      var thresholdX = gyroThresholdXSlider ? gyroThresholdXSlider.value : '3';
      var thresholdY = gyroThresholdYSlider ? gyroThresholdYSlider.value : '5';
      var thresholdZ = gyroThresholdZSlider ? gyroThresholdZSlider.value : '10';
      var moveTime = minMoveTimeSlider ? minMoveTimeSlider.value : '200';
      
      currentState = 'UPDATING_SETTINGS';
      updateStateDisplay();
      updateStatus('⚙️ Updating Enhanced Gyroscope Settings');
      
      var paramString = 'gyroThresholdX=' + thresholdX + 
                       '&gyroThresholdY=' + thresholdY + 
                       '&gyroThresholdZ=' + thresholdZ + 
                       '&minMoveTime=' + moveTime;
      
      makeRequest(paramString);
    }
    
    // Action functions
    function executeAction(action) {
      console.log('Action: ' + action);
      
      var statusMap = {
        'forward': '⬆️ Moving Forward',
        'backward': '⬇️ Moving Backward',
        'stop': '⏹️ Stopped',
        'center': '🎯 Centering Servos',
        'test': '🔧 Testing Servos',
        'rattle': '🐍 RATTLING ACTIVATED! 🐍'
      };
      
      var stateMap = {
        'forward': 'MANUAL_FORWARD',
        'backward': 'MANUAL_BACKWARD',
        'stop': 'STOPPED',
        'center': 'CENTERING',
        'test': 'TESTING',
        'rattle': 'MANUAL_RATTLE'
      };
      
      currentState = stateMap[action] || 'PROCESSING';
      updateStateDisplay();
      
      updateStatus(statusMap[action] || 'Processing...');
      makeRequest('action=' + action);
      
      // Special effects
      if (action === 'rattle') {
        document.body.style.animation = 'shake 0.3s infinite';
        var rattleDuration = parseInt(rattleDurationSlider.value) * 1000;
        setTimeout(function() {
          document.body.style.animation = '';
          currentState = 'IDLE';
          updateStateDisplay();
          updateStatus('🤖 Rattle Complete');
        }, rattleDuration);
      }
    }
    
    function changeMode() {
      var mode = modeSelect ? modeSelect.value : '0';
      var modeName = mode === '0' ? '🌊 Lateral Undulation' : '🏜️ Sidewinding';
      currentState = 'MODE_CHANGE';
      updateStateDisplay();
      updateStatus('Mode: ' + modeName);
      makeRequest('mode=' + mode);
    }
    
    function updateParameters() {
      var amp = amplitudeInput ? amplitudeInput.value : '30';
      var freq = frequencyInput ? frequencyInput.value : '1.0';
      var phase = phaseInput ? phaseInput.value : '60';
      
      currentState = 'UPDATING_PARAMS';
      updateStateDisplay();
      updateStatus('⚙️ Updating Locomotion Parameters');
      makeRequest('amplitude=' + amp + '&frequency=' + freq + '&phase=' + phase);
    }
    
    function updateRattleParameters() {
      var rattleFreq = rattleFreqSlider ? rattleFreqSlider.value : '80';
      var rattleAmp = rattleAmpSlider ? rattleAmpSlider.value : '21';
      var rattleLift = rattleLiftSlider ? rattleLiftSlider.value : '45';
      var rattleDuration = rattleDurationSlider ? rattleDurationSlider.value : '5';
      
      currentState = 'UPDATING_RATTLE';
      updateStateDisplay();
      updateStatus('🐍 Updating Rattle Configuration');
      
      var paramString = 'rattleFreq=' + rattleFreq + 
                       '&rattleAmp=' + rattleAmp + 
                       '&rattleLift=' + rattleLift + 
                       '&rattleDuration=' + rattleDuration;
      
      makeRequest(paramString);
    }
    
    // Update slider value displays
    function updateSliderDisplays() {
      // Gyro threshold sliders
      if (gyroThresholdXSlider) {
        document.getElementById('gyroThresholdXValue').textContent = gyroThresholdXSlider.value + ' °/s';
        gyroThresholdXSlider.oninput = function() {
          document.getElementById('gyroThresholdXValue').textContent = this.value + ' °/s';
        };
      }
      if (gyroThresholdYSlider) {
        document.getElementById('gyroThresholdYValue').textContent = gyroThresholdYSlider.value + ' °/s';
        gyroThresholdYSlider.oninput = function() {
          document.getElementById('gyroThresholdYValue').textContent = this.value + ' °/s';
        };
      }
      if (gyroThresholdZSlider) {
        document.getElementById('gyroThresholdZValue').textContent = gyroThresholdZSlider.value + ' °/s';
        gyroThresholdZSlider.oninput = function() {
          document.getElementById('gyroThresholdZValue').textContent = this.value + ' °/s';
        };
      }
      if (minMoveTimeSlider) {
        document.getElementById('minMoveTimeValue').textContent = minMoveTimeSlider.value + ' ms';
        minMoveTimeSlider.oninput = function() {
          document.getElementById('minMoveTimeValue').textContent = this.value + ' ms';
        };
      }
      
      // Rattle sliders
      if (rattleFreqSlider) {
        document.getElementById('rattleFreqValue').textContent = rattleFreqSlider.value + ' Hz';
        rattleFreqSlider.oninput = function() {
          document.getElementById('rattleFreqValue').textContent = this.value + ' Hz';
        };
      }
      if (rattleAmpSlider) {
        document.getElementById('rattleAmpValue').textContent = rattleAmpSlider.value + '°';
        rattleAmpSlider.oninput = function() {
          document.getElementById('rattleAmpValue').textContent = this.value + '°';
        };
      }
      if (rattleLiftSlider) {
        document.getElementById('rattleLiftValue').textContent = rattleLiftSlider.value + '°';
        rattleLiftSlider.oninput = function() {
          document.getElementById('rattleLiftValue').textContent = this.value + '°';
        };
      }
      if (rattleDurationSlider) {
        document.getElementById('rattleDurationValue').textContent = rattleDurationSlider.value + ' sec';
        rattleDurationSlider.oninput = function() {
          document.getElementById('rattleDurationValue').textContent = this.value + ' sec';
        };
      }
    }
    
    // Check for gyro data timeout
    function checkGyroTimeout() {
      if (Date.now() - lastGyroUpdate > 5000) { // 5 second timeout
        if (dataStatus) dataStatus.textContent = 'Connection Lost';
        if (dataConnectionElement) {
          dataConnectionElement.textContent = 'TIMEOUT';
          dataConnectionElement.className = 'state-value alert';
        }
        document.getElementById('gyroStatus').className = 'gyro-status';
      }
    }
    
    // Periodic state updates
    function periodicUpdates() {
      checkGyroTimeout();
      // Request state data update
      makeRequest('action=getStateData');
      setTimeout(periodicUpdates, 1000); // Update every 1 second for real-time monitoring
    }
    // Event listeners setup
    function setupEventListeners() {
      // Button event listeners
      var forwardBtn = document.getElementById('forwardBtn');
      var backwardBtn = document.getElementById('backwardBtn');
      var stopBtn = document.getElementById('stopBtn');
      var centerBtn = document.getElementById('centerBtn');
      var testBtn = document.getElementById('testBtn');
      var rattleBtn = document.getElementById('rattleBtn');
      var updateBtn = document.getElementById('updateBtn');
      var updateRattleBtn = document.getElementById('updateRattleBtn');
      
      if (forwardBtn) forwardBtn.addEventListener('click', function() { executeAction('forward'); });
      if (backwardBtn) backwardBtn.addEventListener('click', function() { executeAction('backward'); });
      if (stopBtn) stopBtn.addEventListener('click', function() { executeAction('stop'); });
      if (centerBtn) centerBtn.addEventListener('click', function() { executeAction('center'); });
      if (testBtn) testBtn.addEventListener('click', function() { executeAction('test'); });
      if (rattleBtn) rattleBtn.addEventListener('click', function() { executeAction('rattle'); });
      if (updateBtn) updateBtn.addEventListener('click', updateParameters);
      if (updateRattleBtn) updateRattleBtn.addEventListener('click', updateRattleParameters);
      if (modeSelect) modeSelect.addEventListener('change', changeMode);
      
      // Gyro control listeners
      if (calibrateBtn) calibrateBtn.addEventListener('click', calibrateGyro);
      if (gyroControlBtn) gyroControlBtn.addEventListener('click', toggleGyroControl);
      if (gyroSettingsBtn) gyroSettingsBtn.addEventListener('click', updateGyroSettings);
    }
    
    // Initialize when page loads
    document.addEventListener('DOMContentLoaded', function() {
      console.log('🐍 Enhanced Snake Robot State Monitor Loading...');
      setupEventListeners();
      updateSliderDisplays();
      updateStateDisplay();
      updateStatus('🤖 Enhanced System Ready');
      
      // Start periodic updates
      setTimeout(periodicUpdates, 2000);
      
      // Test connection
      setTimeout(function() {
        makeRequest('test=connection');
      }, 1500);
      
      // Show welcome notification
      showNotification('🎯 Enhanced State Monitor System Loaded!', false);
    });
    
    // Add shake animation
    var shakeStyle = document.createElement('style');
    shakeStyle.textContent = `
      @keyframes shake {
        0%, 100% { transform: translateX(0) rotate(0deg); }
        10%, 30%, 50%, 70%, 90% { transform: translateX(-1px) rotate(-0.5deg); }
        20%, 40%, 60%, 80% { transform: translateX(1px) rotate(0.5deg); }
      }
    `;
    document.head.appendChild(shakeStyle);
  </script>
</body>
</html>
)rawliteral";

// ============================================
// ENHANCED SERVER FUNCTIONS WITH STATE DATA
// ============================================

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleSetParameters() {
  Serial.println("=== 📡 POST Request Received ===");
  
  String response = "";
  
  if (server.hasArg("action")) {
    String action = server.arg("action");
    Serial.println("  action = " + action);
    Serial.println(">> 🎮 Action: " + action);
    
    if (action == "forward") {
      locomotionEnabled = true;
      movementMode = 0;
      forwardDirection = true;
      currentMovementState = "MANUAL_FORWARD";
      response = "⬆️ Moving Forward";
    } else if (action == "backward") {
      locomotionEnabled = true;
      movementMode = 0;
      forwardDirection = false;
      currentMovementState = "MANUAL_BACKWARD";
      response = "⬇️ Moving Backward";
    } else if (action == "stop") {
      locomotionEnabled = false;
      rattlingActive = false;
      testingActive = false;
      currentMovementState = "STOPPED";
      response = "⏹️ All Movement Stopped";
    } else if (action == "center") {
      locomotionEnabled = false;
      rattlingActive = false;
      testingActive = false;
      centerAllServos();
      currentMovementState = "CENTERING";
      response = "🎯 Servos Centered";
    } else if (action == "test") {
      locomotionEnabled = false;
      rattlingActive = false;
      testingActive = true;
      testServoIndex = 0;
      testPosition = 0;
      currentMovementState = "TESTING";
      response = "🔧 Comprehensive Servo Test Started!";
    } else if (action == "rattle") {
      rattleTail();
      currentMovementState = "MANUAL_RATTLE";
      response = String("🐍 RATTLE ATTACK! ") + String(RATTLING_FREQ) + "Hz, " + 
                String(RATTLING_AMPLITUDE) + "°, " + String(RATTLING_DURATION/1000) + "s 🐍";
    } else if (action == "calibrateGyro") {
      startGyroCalibration();
      response = "🔧 Starting Enhanced Gyroscope Calibration - Keep device still!";
    } else if (action == "enableGyroControl") {
      GYRO_CONTROL_ENABLED = true;
      currentMovementState = "GYRO_ENABLED";
      response = "🎮 Enhanced Gyro Control ENABLED";
      Serial.println(">> 🎮 Enhanced Gyroscope control enabled");
    } else if (action == "disableGyroControl") {
      GYRO_CONTROL_ENABLED = false;
      locomotionEnabled = false;
      currentMovementState = "GYRO_DISABLED";
      response = "🎮 Gyro Control DISABLED";
      Serial.println(">> 🎮 Gyroscope control disabled");
    } else if (action == "getStateData") {
      // Enhanced state data response
      response = String("STATE DATA | ") +
                "State:" + currentMovementState + " | " +
                "LastMove:" + lastDetectedMovement + " | " +
                "Counter:" + String(movementCounter) + " | " +
                "GyroX:" + String(lastGyroX, 2) + " | " +
                "GyroY:" + String(lastGyroY, 2) + " | " +
                "GyroZ:" + String(lastGyroZ, 2) + " | " +
                "Magnitude:" + String(currentGyroMagnitude, 2) + " | " +
                "F:" + String(forwardDetections) + " | " +
                "B:" + String(backwardDetections) + " | " +
                "L:" + String(leftDetections) + " | " +
                "R:" + String(rightDetections) + " | " +
                "Z:" + String(rattleDetections) + " | " +
                "Calibrated:" + (isCalibrating ? "CALIBRATING" : "YES") + " | " +
                "LastUpdate:" + String(millis()) + "ms";
    }
  }
  
  // Handle mode changes
  if (server.hasArg("mode")) {
    movementMode = server.arg("mode").toInt();
    String modeName = (movementMode == 0) ? "Lateral Undulation" : "Sidewinding";
    response += " | Mode: " + modeName;
    Serial.println(">> 🌊 Movement mode: " + modeName);
  }
  
  // Handle gyroscope threshold settings
  if (server.hasArg("gyroThresholdX")) {
    GYRO_THRESHOLD_X = constrain(server.arg("gyroThresholdX").toFloat(), 1.0, 300);
    response += " | GyroThresholdX:" + String(GYRO_THRESHOLD_X) + "°/s";
    Serial.println(">> 🎯 Enhanced Gyro X threshold updated: " + String(GYRO_THRESHOLD_X) + "°/s");
  }
  if (server.hasArg("gyroThresholdY")) {
    GYRO_THRESHOLD_Y = constrain(server.arg("gyroThresholdY").toFloat(), 1.0, 300);
    response += " | GyroThresholdY:" + String(GYRO_THRESHOLD_Y) + "°/s";
    Serial.println(">> 🎯 Enhanced Gyro Y threshold updated: " + String(GYRO_THRESHOLD_Y) + "°/s");
  }
  if (server.hasArg("gyroThresholdZ")) {
    GYRO_THRESHOLD_Z = constrain(server.arg("gyroThresholdZ").toFloat(), 5.0, 300);
    response += " | GyroThresholdZ:" + String(GYRO_THRESHOLD_Z) + "°/s";
    Serial.println(">> 🎯 Enhanced Gyro Z threshold updated: " + String(GYRO_THRESHOLD_Z) + "°/s");
  }
  if (server.hasArg("minMoveTime")) {
    MIN_MOVE_TIME = constrain(server.arg("minMoveTime").toInt(), 100, 1000);
    response += " | MinMoveTime:" + String(MIN_MOVE_TIME) + "ms";
    Serial.println(">> ⏱️ Enhanced minimum move time updated: " + String(MIN_MOVE_TIME) + "ms");
  }
  
  // Handle rattle parameter updates
  bool rattleParamsUpdated = false;
  if (server.hasArg("rattleFreq")) {
    RATTLING_FREQ = constrain(server.arg("rattleFreq").toFloat(), 10.0, 80.0);
    response += " | RattleFreq:" + String(RATTLING_FREQ) + "Hz";
    Serial.println(">> 🐍 Rattle frequency updated: " + String(RATTLING_FREQ) + " Hz");
    rattleParamsUpdated = true;
  }
  if (server.hasArg("rattleAmp")) {
    RATTLING_AMPLITUDE = constrain(server.arg("rattleAmp").toFloat(), 5.0, 50.0);
    response += " | RattleAmp:" + String(RATTLING_AMPLITUDE) + "°";
    Serial.println(">> 🐍 Rattle amplitude updated: " + String(RATTLING_AMPLITUDE) + "°");
    rattleParamsUpdated = true;
  }
  if (server.hasArg("rattleLift")) {
    RATTLE_LIFT_ANGLE = constrain(server.arg("rattleLift").toFloat(), 20.0, 70.0);
    response += " | RattleLift:" + String(RATTLE_LIFT_ANGLE) + "°";
    Serial.println(">> 🐍 Rattle lift angle updated: " + String(RATTLE_LIFT_ANGLE) + "°");
    rattleParamsUpdated = true;
  }
  if (server.hasArg("rattleDuration")) {
    RATTLING_DURATION = constrain(server.arg("rattleDuration").toInt(), 2, 10) * 1000;
    response += " | RattleDur:" + String(RATTLING_DURATION/1000) + "s";
    Serial.println(">> 🐍 Rattle duration updated: " + String(RATTLING_DURATION/1000) + " seconds");
    rattleParamsUpdated = true;
  }
  
  if (rattleParamsUpdated) {
    response = "🐍 ENHANCED RATTLE SETTINGS UPDATED! " + response;
    Serial.printf(">> 🐍 New Enhanced Rattle Config: %.1fHz, %.1f°, %.1f° lift, %lus\n", 
                  RATTLING_FREQ, RATTLING_AMPLITUDE, RATTLE_LIFT_ANGLE, RATTLING_DURATION/1000);
  }

  // Handle locomotion parameters
  if (server.hasArg("amplitude")) {
    amplitude = constrain(server.arg("amplitude").toFloat(), 10.0, 45.0);
    response += " | Amp:" + String(amplitude) + "°";
    Serial.println(">> ⚙️ Amplitude: " + String(amplitude) + "°");
  }
  if (server.hasArg("frequency")) {
    frequency = constrain(server.arg("frequency").toFloat(), 0.1, 2.0);
    response += " | Freq:" + String(frequency) + "Hz";
    Serial.println(">> ⚙️ Frequency: " + String(frequency) + " Hz");
  }
  if (server.hasArg("phase")) {
    phaseOffset = constrain(server.arg("phase").toFloat(), 30.0, 90.0);
    response += " | Phase:" + String(phaseOffset) + "°";
    Serial.println(">> ⚙️ Phase offset: " + String(phaseOffset) + "°");
  }
  
  if (response == "") response = "❓ Unknown Command";
  
  Serial.println(">> 📤 Response: " + response);
  Serial.println("=== End Request ===");
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "text/plain", response);
}

// ============================================
// SERVO CONTROL FUNCTIONS
// ============================================

void centerAllServos() {
  Serial.println("🎯 Centering all servos...");
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].write(centerPosition);
    delay(10);
  }
  Serial.println("✅ All servos centered at " + String(centerPosition) + "°");
}



void rattleTail() {
  if (rattlingActive) {
    Serial.println("⚠️ Rattle already active, ignoring command");
    return;
  }
  
  Serial.println("🐍 ========================================");
  Serial.println("🐍 INITIATING ENHANCED RATTLE SEQUENCE!");
  Serial.printf("🐍 Config: %.1fHz, %.1f°, %.1f° lift, %lus\n", 
                RATTLING_FREQ, RATTLING_AMPLITUDE, RATTLE_LIFT_ANGLE, RATTLING_DURATION/1000);
  Serial.println("🐍 ========================================");
  
  rattlingActive = true;
  rattlingStart = millis();
  bodyHasCurled = false;
  currentMovementState = "RATTLING";
  
  // Stop other movements
  locomotionEnabled = false;
  testingActive = false;
  
  // Lift and position tail for optimal rattling
  servos[tailLiftIdx].write(centerPosition + RATTLE_LIFT_ANGLE);
  delay(200);
  
  Serial.println("🐍 Tail positioned, starting enhanced rattle attack!");
}

void updateTailRattle() {
  if (!rattlingActive) return;
  
  unsigned long currentTime = millis();
  
  // Check if rattle duration is complete
  if (currentTime - rattlingStart >= RATTLING_DURATION) {
    Serial.println("🐍 ========================================");
    Serial.println("🐍 ENHANCED RATTLE SEQUENCE COMPLETE!");
    Serial.printf("🐍 Duration: %lu seconds\n", (currentTime - rattlingStart) / 1000);
    Serial.println("🐍 ========================================");
    
    rattlingActive = false;
    bodyHasCurled = false;
    currentMovementState = "IDLE";
    
    // Return servos to center
    servos[tailLiftIdx].write(centerPosition);
    servos[tailRattle1Idx].write(centerPosition);
    
    Serial.println("🐍 Tail returned to rest position");
    return;
  }
  
  // Body curl phase (first 500ms)
  if (!bodyHasCurled && (currentTime - rattlingStart < 500)) {
    float curlProgress = (currentTime - rattlingStart) / 500.0;
    
    for (int i = 0; i < NUM_SERVOS - 2; i++) {
      if (servoLayout[i] == 1) { // Horizontal servos
        float curlAngle = centerPosition + (CURL_AMPLITUDE * curlProgress * 
                          sin(PI * i / (NUM_SERVOS - 2)));
        servos[i].write(constrain(curlAngle, 45, 135));
      }
    }
    
    if (curlProgress >= 1.0) {
      bodyHasCurled = true;
      Serial.println("🐍 Body curl complete, starting tail rattle!");
    }
  }
  
  // High-speed tail rattle
  if (bodyHasCurled && (currentTime - lastRattleUpdate >= RATTLE_UPDATE_INTERVAL)) {
    float timeInSeconds = (currentTime - rattlingStart) / 1000.0;
    float rattleAngle = RATTLING_AMPLITUDE * sin(2 * PI * RATTLING_FREQ * timeInSeconds);
    
    int targetAngle = constrain(centerPosition + rattleAngle, 45, 135);
    servos[tailRattle1Idx].write(targetAngle);
    
    lastRattleUpdate = currentTime;
  }
}

void handleServoTest() {
  if (!testingActive) return;
  
  unsigned long currentTime = millis();
  
  if (currentTime - lastTestMillis >= 100) {
    servos[testServoIndex].write(testPosition);
    
    Serial.printf("🔧 Testing Servo %d: Position %d°\n", testServoIndex + 1, testPosition);
    
    testPosition += 10;
    if (testPosition > 180) {
      testPosition = 0;
      testServoIndex++;
      
      if (testServoIndex >= NUM_SERVOS) {
        testingActive = false;
        testServoIndex = 0;
        currentMovementState = "IDLE";
        centerAllServos();
        Serial.println("✅ Servo test sequence complete!");
      }
    }
    
    lastTestMillis = currentTime;
  }
}

void updateServosLateralUndulation() {
  unsigned long currentTime = millis();
  float t = currentTime / 1000.0;
  
  for (int i = 0; i < NUM_SERVOS; i++) {
    if (servoLayout[i] == 1) {
      float phase = (PI * phaseOffset / 180.0) * i;
      if (!forwardDirection) phase = -phase;
      
      float angle = centerPosition + amplitude * sin(2 * PI * frequency * t + phase);
      servos[i].write(constrain(angle, 45, 135));
    } else {
      float verticalPhase = (PI * verticalPhaseOffset / 180.0) * i;
      float verticalAngle = centerPosition + verticalAmplitude * sin(2 * PI * frequency * t + verticalPhase);
      servos[i].write(constrain(verticalAngle, 45, 135));
    }
  }
}

void updateServosSidewinding() {
  unsigned long currentTime = millis();
  float t = currentTime / 1000.0;
  
  for (int i = 0; i < NUM_SERVOS; i++) {
    float horizontalPhase = (PI * phaseOffset / 180.0) * i;
    float verticalPhase = horizontalPhase + PI/2;
    
    if (!forwardDirection) {
      horizontalPhase = -horizontalPhase;
      verticalPhase = -verticalPhase;
    }
    
    if (servoLayout[i] == 1) {
      float angle = centerPosition + amplitude * sin(2 * PI * frequency * t + horizontalPhase);
      servos[i].write(constrain(angle, 45, 135));
    } else {
      float verticalAngle = centerPosition + verticalAmplitude * sin(2 * PI * frequency * t + verticalPhase);
      servos[i].write(constrain(verticalAngle, 45, 135));
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("🐍========================================🐍");
  Serial.println("🚀 ENHANCED SNAKE ROBOT WITH STATE MONITOR 🚀");
  Serial.println("🐍========================================🐍");
  Serial.printf("📅 Date: 2025-06-17 11:04:40 UTC\n");
  Serial.printf("👨‍💻 Developer: AmrEslim\n");
  Serial.printf("🔧 Version: 2.3 Enhanced State Monitoring\n");
  Serial.println("🐍========================================🐍");
  
  // Initialize regular servos
  Serial.println("⚙️ Initializing enhanced servo systems...");
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(centerPosition);
    Serial.printf("  ✅ Servo %d: Pin %d initialized\n", i+1, servoPins[i]);
    delay(50);
  }
  
  // Setup enhanced rattle system
  Serial.println("🐍 Initializing enhanced rattle system...");
  setupRattlePWM();
  
  // Initialize WiFi for both AP and ESP-NOW
  Serial.println("📡 Configuring enhanced WiFi systems...");
  WiFi.mode(WIFI_AP_STA);
  
  // Setup ESP-NOW with enhanced error handling
  Serial.println("🎯 Initializing Enhanced ESP-NOW for gyroscope control...");
  esp_err_t initResult = esp_now_init();
  if (initResult != ESP_OK) {
    Serial.print("❌ ESP-NOW init FAILED with error: ");
    Serial.println(initResult);
    currentMovementState = "ESP_NOW_FAILED";
  } else {
    Serial.println("✅ ESP-NOW initialized successfully");
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("✅ ESP-NOW receiver callback registered");
    currentMovementState = "IDLE";
  }
  
  // Setup WiFi AP for enhanced web control
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP("ESP32_Snake_Enhanced", "SnakeBot2025");
  delay(2000);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.println("🐍 Enhanced Snake Robot AP Started!");
  Serial.printf("🌐 IP Address: %s\n", IP.toString().c_str());
  Serial.println("📱 Network: ESP32_Snake_Enhanced");
  Serial.println("🔐 Password: SnakeBot2025");
  Serial.printf("🌍 Enhanced Control Panel: http://%s\n", IP.toString().c_str());
  Serial.print("📻 ESP-NOW MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Start enhanced web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/", HTTP_POST, handleSetParameters);
  server.onNotFound([]() {
    server.send(404, "text/plain", "🐍 Page not found in enhanced snake territory!");
  });
  
  server.begin();
  Serial.println("🎮 Enhanced HTTP Control Panel Started!");
  Serial.println("🐍========================================🐍");
  Serial.println("🤖 ENHANCED GYRO-ENABLED SNAKE ROBOT READY! 🤖");
  Serial.printf("🎯 Enhanced Gyro Thresholds: X:%.1f Y:%.1f Z:%.1f °/s\n", 
                GYRO_THRESHOLD_X, GYRO_THRESHOLD_Y, GYRO_THRESHOLD_Z);
  Serial.printf("⏱️ Enhanced Min Move Time: %lu ms\n", MIN_MOVE_TIME);
  Serial.printf("🐍 Enhanced Rattle Config: %.1fHz, %.1f°, %.1f° lift, %lus\n", 
                RATTLING_FREQ, RATTLING_AMPLITUDE, RATTLE_LIFT_ANGLE, RATTLING_DURATION/1000);
  Serial.println("🎯 Enhanced Gyroscope Control: Waiting for calibration...");
  Serial.println("📊 Real-Time State Monitoring: ACTIVE");
  Serial.println("🐍========================================🐍");
}

void loop() {
  server.handleClient();
  
  // Check enhanced gyro timeout
  if (GYRO_CONTROL_ENABLED && (millis() - lastGyroUpdate > GYRO_TIMEOUT)) {
    if (currentMovementState != "GYRO_TIMEOUT") {
      Serial.println(">>> 🎯 ENHANCED GYRO TIMEOUT - Stopping movement");
      currentMovementState = "GYRO_TIMEOUT";
    }
    locomotionEnabled = false;
    centerAllServos();
  }
  
  // Priority-based enhanced servo control
  if (rattlingActive) {
    updateTailRattle();
  } else if (testingActive) {
    handleServoTest();
  } else if (locomotionEnabled && (millis() - lastUpdate >= interval)) {
    if (movementMode == 0) {
      updateServosLateralUndulation();
    } else {
      updateServosSidewinding();
    }
    lastUpdate = millis();
  }
  
  // Update movement state based on current activity
  if (!rattlingActive && !testingActive && !locomotionEnabled && 
      GYRO_CONTROL_ENABLED && currentMovementState.indexOf("MANUAL_") == -1 && 
      currentMovementState != "GYRO_TIMEOUT") {
    currentMovementState = "IDLE";
  }
  
  // Yield to prevent watchdog issues
  yield();
}