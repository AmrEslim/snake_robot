#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <math.h>

WebServer server(80);

// Servo configuration
const int NUM_SERVOS = 10;
const int NUM_HORIZONTAL = 7;
const int servoLayout[NUM_SERVOS] = {1, 0, 1, 1, 0, 1, 1, 0, 1, 1};
const int servoPins[NUM_SERVOS] = {23, 22, 2, 4, 16, 17, 5, 18, 19, 21};

// Enhanced Rattling configuration
const int tailLiftIdx = 7;
const int tailRattle1Idx = 9;
// const int tailRattle2Idx = 9;

// PWM configuration for high-speed rattling (FIXED for new ESP32 core)
const int PWM_FREQ = 50; // 50Hz for servos
const int PWM_RESOLUTION = 16; // 16-bit resolution

bool rattlingActive = false;
unsigned long rattlingStart = 0;
unsigned long RATTLING_DURATION = 5000; // Now adjustable
float RATTLING_FREQ = 80.0; // Much higher frequency - adjustable
float RATTLING_AMPLITUDE = 21.0; // Larger amplitude - adjustable
float RATTLE_LIFT_ANGLE = 45.0; // How high to lift tail - adjustable
const float CURL_AMPLITUDE = 70.0;
bool bodyHasCurled = false;
unsigned long lastRattleUpdate = 0;
const int RATTLE_UPDATE_INTERVAL = 5; // Fixed to 10ms for better vibration

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

// Fixed HTML with working rattle parameter controls
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>🐍 Snake Robot Control v2.0</title>
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
      max-width: 1100px;
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
    .btn-rattle {
      background: linear-gradient(45deg, #8b5cf6, #a78bfa, #c084fc);
      color: white;
      font-size: 1.1rem;
      animation: rattleGlow 2s infinite;
      border: 2px solid #7c3aed;
      font-weight: 900;
      letter-spacing: 1px;
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
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>🐍 SNAKE ROBOT CONTROL 🐍</h1>
      <div class="subtitle">Advanced Bio-Inspired Locomotion System</div>
      <div class="timestamp">Updated: 2025-01-16 14:27 UTC | By AmrEslim</div>
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
      
      <div class="rattle-controls">
        <div class="rattle-title">🐍 RATTLE CONFIGURATION 🐍</div>
        <div class="param-grid">
          <div class="param-group">
            <label class="param-label">⚡ Rattle Frequency</label>
            <div class="param-description">How fast the tail vibrates (10-80 Hz)</div>
            <div class="slider-container">
              <input type="range" id="rattleFreq" class="slider" min="10" max="80" step="5" value="50">
              <span class="value-display" id="rattleFreqValue">50 Hz</span>
            </div>
          </div>
          <div class="param-group">
            <label class="param-label">📐 Rattle Amplitude</label>
            <div class="param-description">How wide the tail swings (5-50°)</div>
            <div class="slider-container">
              <input type="range" id="rattleAmp" class="slider" min="5" max="50" step="2" value="35">
              <span class="value-display" id="rattleAmpValue">35°</span>
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
      <p>🚀 Snake Robot v2.0 Enhanced | Developed by AmrEslim | 2025 🚀</p>
      <p>🐍 Advanced Bio-Inspired Robotics System 🐍</p>
    </div>
  </div>

  <div class="notification" id="notification"></div>

  <script>
    // Global variables
    var currentStatus = 'Ready for Commands';
    
    // DOM elements
    var statusElement = document.getElementById('statusText');
    var notificationElement = document.getElementById('notification');
    var modeSelect = document.getElementById('modeSelect');
    var amplitudeInput = document.getElementById('amplitude');
    var frequencyInput = document.getElementById('frequency');
    var phaseInput = document.getElementById('phase');
    
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
      
      updateStatus(statusMap[action] || 'Processing...');
      makeRequest('action=' + action);
      
      // Special effects
      if (action === 'rattle') {
        document.body.style.animation = 'shake 0.3s infinite';
        var rattleDuration = parseInt(rattleDurationSlider.value) * 1000;
        setTimeout(function() {
          document.body.style.animation = '';
          updateStatus('🤖 Rattle Complete');
        }, rattleDuration);
      }
    }
    
    function changeMode() {
      var mode = modeSelect ? modeSelect.value : '0';
      var modeName = mode === '0' ? '🌊 Lateral Undulation' : '🏜️ Sidewinding';
      updateStatus('Mode: ' + modeName);
      makeRequest('mode=' + mode);
    }
    
    function updateParameters() {
      var amp = amplitudeInput ? amplitudeInput.value : '30';
      var freq = frequencyInput ? frequencyInput.value : '1.0';
      var phase = phaseInput ? phaseInput.value : '60';
      
      updateStatus('⚙️ Updating Locomotion Parameters');
      makeRequest('amplitude=' + amp + '&frequency=' + freq + '&phase=' + phase);
    }
    
    // FIXED: This function now properly sends rattle parameters
    function updateRattleParameters() {
      var rattleFreq = rattleFreqSlider ? rattleFreqSlider.value : '50';
      var rattleAmp = rattleAmpSlider ? rattleAmpSlider.value : '35';
      var rattleLift = rattleLiftSlider ? rattleLiftSlider.value : '45';
      var rattleDuration = rattleDurationSlider ? rattleDurationSlider.value : '5';
      
      console.log('Sending rattle params:', {
        freq: rattleFreq,
        amp: rattleAmp,
        lift: rattleLift,
        duration: rattleDuration
      });
      
      updateStatus('🐍 Updating Rattle Configuration');
      
      // Send rattle parameters with correct parameter names
      var paramString = 'rattleFreq=' + rattleFreq + 
                       '&rattleAmp=' + rattleAmp + 
                       '&rattleLift=' + rattleLift + 
                       '&rattleDuration=' + rattleDuration;
      
      makeRequest(paramString);
    }
    
    // Update slider value displays
    function updateSliderDisplays() {
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
      if (updateRattleBtn) {
        updateRattleBtn.addEventListener('click', function() {
          console.log('Rattle update button clicked!');
          updateRattleParameters();
        });
      }
      if (modeSelect) modeSelect.addEventListener('change', changeMode);
    }
    
    // Initialize when page loads
    document.addEventListener('DOMContentLoaded', function() {
      console.log('🐍 Snake Robot Control Panel Loading...');
      setupEventListeners();
      updateSliderDisplays();
      updateStatus('🤖 Ready for Commands');
      
      // Test connection
      setTimeout(function() {
        makeRequest('test=connection');
      }, 1500);
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

void handleRoot() {
  Serial.println(">> Serving enhanced control panel...");
  server.send(200, "text/html", htmlPage);
}

// FIXED: Simplified approach using rapid servo movements for rattle effect
void setupRattlePWM() {
  Serial.println(">> Setting up high-speed rattle system...");
  Serial.printf("   Rattle motors: Pin %d and Pin %d\n", servoPins[tailRattle1Idx]);
  Serial.println(">> Rattle system ready - using rapid servo control");
}

void rattleTail() {
  rattlingActive = true;
  rattlingStart = millis();
  locomotionEnabled = false;
  testingActive = false;
  bodyHasCurled = false;
  lastRattleUpdate = 0;
  Serial.printf("*** 🐍 RATTLE ATTACK ACTIVATED! Freq: %.1f Hz, Amp: %.1f°, Duration: %lu ms ***\n", 
                RATTLING_FREQ, RATTLING_AMPLITUDE, RATTLING_DURATION);
}

// FIXED: Using rapid servo writes for reliable rattle effect
void updateTailRattle() {
  if (!rattlingActive) return;
  
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - rattlingStart;
  
  if (elapsed > RATTLING_DURATION) {
    rattlingActive = false;
    bodyHasCurled = false;
    
    // Center all servos
    for (int i = 0; i < NUM_SERVOS; i++) {
      servos[i].write(centerPosition);
    }
    Serial.println("*** 🐍 Rattle attack complete - all systems normal ***");
    return;
  }
  
  // High-frequency rattle updates
  if (currentTime - lastRattleUpdate >= RATTLE_UPDATE_INTERVAL) {
    lastRattleUpdate = currentTime;
    
    float t = elapsed / 1000.0f;
    
    // Body curl logic - enhanced with smoother transition
    if (!bodyHasCurled && elapsed < 800) {
      for (int i = 0; i < 6; i++) { // Include one more servo for better curl
        if (servoLayout[i] == 1) {
          float curlProgress = elapsed / 800.0f;
          // Use easing function for smoother curl
          float easedProgress = curlProgress * curlProgress * (3.0f - 2.0f * curlProgress);
          float curlAngle = centerPosition + CURL_AMPLITUDE * sin(easedProgress * PI) * 0.9;
          servos[i].write(constrain(curlAngle, 0, 180));
        }
      }
    } else if (elapsed >= 800 && !bodyHasCurled) {
      bodyHasCurled = true;
      for (int i = 0; i < 6; i++) {
        if (servoLayout[i] == 1) {
          float finalCurlAngle = centerPosition + CURL_AMPLITUDE * 0.7;
          servos[i].write(constrain(finalCurlAngle, 0, 180));
        }
      }
      Serial.println("*** 🐍 Body coiled in attack position - maximum rattle intensity! ***");
    }
    
    // Enhanced tail lift with dynamic adjustment
    float dynamicLift = RATTLE_LIFT_ANGLE + 5.0 * sin(2 * PI * 0.5 * t); // Slight lift variation
    float liftAngle = centerPosition - dynamicLift;
    servos[tailLiftIdx].write(constrain(liftAngle, 0, 180));
    
    // HIGH-SPEED servo-based rattling - FIXED approach
    float phase1 = 2 * PI * RATTLING_FREQ * t;
    float phase2 = phase1 + PI; // 180° out of phase for maximum effect
    
    // Add harmonic for more realistic rattle effect
    float harmonic = 0.2 * sin(phase1 * 2); // Reduced harmonic for stability
    
    float rattle1Angle = centerPosition + RATTLING_AMPLITUDE * (sin(phase1) + harmonic);
    //float rattle2Angle = centerPosition + RATTLING_AMPLITUDE * (sin(phase2) + harmonic);
    
    // Use servo library for reliable movement
    servos[tailRattle1Idx].write(constrain(rattle1Angle, 0, 180));
    // servos[tailRattle2Idx].write(constrain(rattle2Angle, 0, 180));
    
    // Debug output every 100ms
    if (elapsed % 100 < RATTLE_UPDATE_INTERVAL) {
      Serial.printf("🐍 Rattling: %.1f° | %.1f°\n", rattle1Angle);
    }
  }
}

void centerAllServos() {
  locomotionEnabled = false;
  testingActive = false;
  rattlingActive = false;
  bodyHasCurled = false;
  Serial.println(">> 🎯 Centering all servos to home position...");
  
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].write(centerPosition);
  }
  Serial.println(">> ✅ All servos centered and ready!");
}

void handleServoTest() {
  if (millis() - lastTestMillis >= 400) {
    lastTestMillis = millis();
    
    Serial.printf("Testing Servo %d - Position %d\n", testServoIndex + 1, testPosition);
    
    switch (testPosition) {
      case 0: servos[testServoIndex].write(45); break;
      case 1: servos[testServoIndex].write(90); break;
      case 2: servos[testServoIndex].write(135); break;
      case 3: servos[testServoIndex].write(90); break;
    }

    if (++testPosition > 3) {
      testPosition = 0;
      if (++testServoIndex >= NUM_SERVOS) {
        testingActive = false;
        testServoIndex = 0;
        centerAllServos();
        Serial.println(">> ✅ Complete servo test finished successfully!");
      }
    }
  }
}

void updateServosLateralUndulation() {
  float timeScale = millis() * 0.001 * frequency * 2 * PI;
  int horizontalIndex = 0;
  
  for (int i = 0; i < NUM_SERVOS; i++) {
    if (servoLayout[i] == 1) {
      float segmentPhase = forwardDirection ? 
        (NUM_HORIZONTAL - 1 - horizontalIndex) * radians(phaseOffset) :
        horizontalIndex * radians(phaseOffset);
      
      float angle = centerPosition + amplitude * sin(timeScale + segmentPhase);
      servos[i].write(constrain(angle, 0, 180));
      horizontalIndex++;
    } else {
      servos[i].write(centerPosition);
    }
  }
}

void updateServosSidewinding() {
  float timeScale = millis() * 0.001 * frequency * 2 * PI;
  int horizontalIndex = 0;
  int verticalIndex = 0;

  for (int i = 0; i < NUM_SERVOS; i++) {
    if (servoLayout[i] == 1) {
      float phase = forwardDirection ? 
        radians((NUM_HORIZONTAL - 1 - horizontalIndex) * phaseOffset) : 
        radians(horizontalIndex * phaseOffset);
      
      float angle = centerPosition + amplitude * sin(timeScale + phase);
      servos[i].write(constrain(angle, 0, 180));
      horizontalIndex++;
    } else {
      float basePhase = verticalIndex * phaseOffset + verticalPhaseOffset;
      float phase = forwardDirection ? 
        radians((NUM_SERVOS - NUM_HORIZONTAL - 1 - verticalIndex) * phaseOffset + verticalPhaseOffset) : 
        radians(basePhase);
      
      float angle = centerPosition + verticalAmplitude * sin(timeScale + phase);
      servos[i].write(constrain(angle, 0, 180));
      verticalIndex++;
    }
  }
}

void handleSetParameters() {
  String response = "";
  
  Serial.println("=== 📡 POST Request Received ===");
  for (int i = 0; i < server.args(); i++) {
    Serial.println("  " + server.argName(i) + " = " + server.arg(i));
  }
  
  if (server.hasArg("test")) {
    response = "🤖 Connection Established Successfully!";
    Serial.println(">> 🔗 Connection test successful");
  }
  
  if (server.hasArg("mode")) {
    movementMode = server.arg("mode").toInt();
    response = "🎯 Mode: " + String(movementMode == 0 ? "🌊 Lateral Undulation" : "🏜️ Sidewinding");
    Serial.println(">> 🎯 Movement mode: " + String(movementMode));
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
    response = "🐍 RATTLE SETTINGS UPDATED! " + response;
    Serial.printf(">> 🐍 New Rattle Config: %.1fHz, %.1f°, %.1f° lift, %lus\n", 
                  RATTLING_FREQ, RATTLING_AMPLITUDE, RATTLE_LIFT_ANGLE, RATTLING_DURATION/1000);
  }
  
  if (server.hasArg("action")) {
    String action = server.arg("action");
    Serial.println(">> 🎮 Action: " + action);
    
    if (action == "center") {
      centerAllServos();
      response = "🎯 All Servos Centered!";
    } else if (action == "forward") {
      locomotionEnabled = true;
      forwardDirection = true;
      rattlingActive = false;
      response = "⬆️ Forward Locomotion Activated!";
    } else if (action == "backward") {
      locomotionEnabled = true;
      forwardDirection = false;
      rattlingActive = false;
      response = "⬇️ Backward Locomotion Activated!";
    } else if (action == "stop") {
      locomotionEnabled = false;
      rattlingActive = false;
      centerAllServos();
      response = "⏹️ All Systems Stopped!";
    } else if (action == "test") {
      testingActive = true;
      rattlingActive = false;
      testServoIndex = 0;
      testPosition = 0;
      response = "🔧 Comprehensive Servo Test Started!";
    } else if (action == "rattle") {
      rattleTail();
      response = String("🐍 RATTLE ATTACK! ") + String(RATTLING_FREQ) + "Hz, " + 
                String(RATTLING_AMPLITUDE) + "°, " + String(RATTLING_DURATION/1000) + "s 🐍";
    }
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

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("🐍========================================🐍");
  Serial.println("🚀   ENHANCED SNAKE ROBOT STARTING   🚀");
  Serial.println("🐍========================================🐍");
  Serial.printf("📅 Date: 2025-01-16 14:27 UTC\n");
  Serial.printf("👨‍💻 Developer: AmrEslim\n");
  Serial.printf("🔧 Version: 2.0 Enhanced - SERVO RATTLE\n");
  Serial.println("🐍========================================🐍");
  
  // Initialize regular servos
  Serial.println("⚙️ Initializing servo systems...");
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(centerPosition);
    Serial.printf("  ✅ Servo %d: Pin %d initialized\n", i+1, servoPins[i]);
    delay(50);
  }
  
  // Setup rattle system - FIXED to use servo-based approach
  Serial.println("🐍 Initializing advanced rattle system...");
  setupRattlePWM();
  
  // Start WiFi AP
  Serial.println("📡 Configuring WiFi Access Point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP("ESP32_Snake_Enhanced", "SnakeBot2025");
  delay(2000);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.println("🐍 Enhanced Snake Robot AP Started!");
  Serial.printf("🌐 IP Address: %s\n", IP.toString().c_str());
  Serial.println("📱 Network: ESP32_Snake_Enhanced");
  Serial.println("🔐 Password: SnakeBot2025");
  Serial.printf("🌍 Control Panel: http://%s\n", IP.toString().c_str());

  // Start enhanced web server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/", HTTP_POST, handleSetParameters);
  server.onNotFound([]() {
    server.send(404, "text/plain", "🐍 Page not found in snake territory!");
  });
  
  server.begin();
  Serial.println("🎮 Enhanced HTTP Control Panel Started!");
  Serial.println("🐍========================================🐍");
  Serial.println("🤖 ENHANCED SNAKE ROBOT READY FOR ACTION! 🤖");
  Serial.printf("🐍 Initial Rattle Config: %.1fHz, %.1f°, %.1f° lift, %lus 🐍\n", 
                RATTLING_FREQ, RATTLING_AMPLITUDE, RATTLE_LIFT_ANGLE, RATTLING_DURATION/1000);
  Serial.println("🐍========================================🐍");
}

void loop() {
  server.handleClient();
  
  // Priority-based servo control
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
  
  // Yield to prevent watchdog issues
  yield();
}