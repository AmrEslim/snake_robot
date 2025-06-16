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

// Rattling configuration
const int tailLiftIdx = 7;
const int tailRattle1Idx = 8;
const int tailRattle2Idx = 9;

bool rattlingActive = false;
unsigned long rattlingStart = 0;
const unsigned long RATTLING_DURATION = 5000;
const float RATTLING_FREQ = 30.0;
const float RATTLING_AMPLITUDE = 45.0;
const float CURL_AMPLITUDE = 30.0;
bool bodyHasCurled = false;

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

// HTML with completely rewritten JavaScript
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>Snake Robot Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }
    body {
      font-family: Arial, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      padding: 20px;
      color: #333;
    }
    .container {
      max-width: 900px;
      margin: 0 auto;
    }
    .header {
      text-align: center;
      color: white;
      margin-bottom: 30px;
    }
    .header h1 {
      font-size: 2.5rem;
      margin-bottom: 10px;
      text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
    }
    .card {
      background: white;
      border-radius: 15px;
      padding: 25px;
      margin-bottom: 20px;
      box-shadow: 0 10px 30px rgba(0,0,0,0.2);
    }
    .card-title {
      font-size: 1.3rem;
      font-weight: bold;
      margin-bottom: 20px;
      color: #333;
    }
    .control-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 15px;
      margin-bottom: 20px;
    }
    .btn {
      padding: 15px 20px;
      border: none;
      border-radius: 10px;
      font-size: 1rem;
      font-weight: bold;
      cursor: pointer;
      transition: all 0.3s ease;
      text-transform: uppercase;
    }
    .btn:hover {
      transform: translateY(-2px);
      box-shadow: 0 5px 15px rgba(0,0,0,0.3);
    }
    .btn-primary { background: #3b82f6; color: white; }
    .btn-success { background: #10b981; color: white; }
    .btn-danger { background: #ef4444; color: white; }
    .btn-warning { background: #f59e0b; color: white; }
    .btn-rattle {
      background: linear-gradient(45deg, #8b5cf6, #a78bfa);
      color: white;
      font-size: 1.1rem;
      animation: glow 2s infinite;
    }
    @keyframes glow {
      0%, 100% { box-shadow: 0 0 10px rgba(139, 92, 246, 0.5); }
      50% { box-shadow: 0 0 20px rgba(139, 92, 246, 0.8); }
    }
    select, input {
      width: 100%;
      padding: 12px;
      border: 2px solid #ddd;
      border-radius: 8px;
      font-size: 1rem;
      margin-bottom: 10px;
    }
    select:focus, input:focus {
      outline: none;
      border-color: #3b82f6;
    }
    .param-group {
      margin-bottom: 15px;
    }
    .param-label {
      display: block;
      margin-bottom: 5px;
      font-weight: bold;
      color: #555;
    }
    .status {
      background: linear-gradient(135deg, #e0f2fe, #f0f9ff);
      border: 2px solid #0ea5e9;
      border-radius: 10px;
      padding: 20px;
      text-align: center;
      margin-top: 20px;
    }
    .status-text {
      font-size: 1.2rem;
      font-weight: bold;
      color: #0369a1;
    }
    .notification {
      position: fixed;
      top: 20px;
      right: 20px;
      background: #10b981;
      color: white;
      padding: 15px 25px;
      border-radius: 10px;
      font-weight: bold;
      transform: translateX(400px);
      transition: transform 0.3s ease;
      z-index: 1000;
      max-width: 300px;
    }
    .notification.show {
      transform: translateX(0);
    }
    .notification.error {
      background: #ef4444;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <h1>🐍 SNAKE ROBOT CONTROL 🐍</h1>
      <p>Advanced Locomotion System</p>
    </div>

    <div class="card">
      <div class="card-title">🎯 Movement Mode</div>
      <select id="modeSelect">
        <option value="0">🌊 Lateral Undulation</option>
        <option value="1">🏜️ Sidewinding</option>
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
      <div class="card-title">⚙️ Motion Parameters</div>
      <div class="param-group">
        <label class="param-label">📏 Amplitude (10-45°)</label>
        <input type="number" id="amplitude" min="10" max="45" step="1" value="30">
      </div>
      <div class="param-group">
        <label class="param-label">⚡ Frequency (0.1-2.0 Hz)</label>
        <input type="number" id="frequency" min="0.1" max="2.0" step="0.1" value="1.0">
      </div>
      <div class="param-group">
        <label class="param-label">🌊 Phase Offset (30-90°)</label>
        <input type="number" id="phase" min="30" max="90" step="5" value="60">
      </div>
      <button class="btn btn-success" id="updateBtn">💾 Update Parameters</button>
    </div>

    <div class="status">
      <div class="status-text">🤖 Status: <span id="statusText">Ready</span></div>
    </div>
  </div>

  <div class="notification" id="notification"></div>

  <script>
    // Global variables
    var currentStatus = 'Ready';
    
    // DOM elements
    var statusElement = document.getElementById('statusText');
    var notificationElement = document.getElementById('notification');
    var modeSelect = document.getElementById('modeSelect');
    var amplitudeInput = document.getElementById('amplitude');
    var frequencyInput = document.getElementById('frequency');
    var phaseInput = document.getElementById('phase');
    
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
      }, 4000);
    }
    
    // HTTP request function
    function makeRequest(data) {
      console.log('Making request: ' + data);
      
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
            showNotification('Error: Request failed', true);
          }
        }
      };
      
      xhr.onerror = function() {
        console.error('Network error');
        showNotification('Network error', true);
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
        'center': '🎯 Centering',
        'test': '🔧 Testing',
        'rattle': '🐍 RATTLING! 🐍'
      };
      
      updateStatus(statusMap[action] || 'Processing...');
      makeRequest('action=' + action);
      
      // Special effect for rattle
      if (action === 'rattle') {
        document.body.style.animation = 'shake 0.5s infinite';
        setTimeout(function() {
          document.body.style.animation = '';
        }, 5000);
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
      
      updateStatus('⚙️ Updating Parameters');
      makeRequest('amplitude=' + amp + '&frequency=' + freq + '&phase=' + phase);
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
      
      if (forwardBtn) forwardBtn.addEventListener('click', function() { executeAction('forward'); });
      if (backwardBtn) backwardBtn.addEventListener('click', function() { executeAction('backward'); });
      if (stopBtn) stopBtn.addEventListener('click', function() { executeAction('stop'); });
      if (centerBtn) centerBtn.addEventListener('click', function() { executeAction('center'); });
      if (testBtn) testBtn.addEventListener('click', function() { executeAction('test'); });
      if (rattleBtn) rattleBtn.addEventListener('click', function() { executeAction('rattle'); });
      if (updateBtn) updateBtn.addEventListener('click', updateParameters);
      if (modeSelect) modeSelect.addEventListener('change', changeMode);
    }
    
    // Initialize when page loads
    document.addEventListener('DOMContentLoaded', function() {
      console.log('Page loaded - initializing...');
      setupEventListeners();
      updateStatus('🤖 Ready for Commands');
      
      // Test connection
      setTimeout(function() {
        makeRequest('test=connection');
      }, 1000);
    });
    
    // Add shake animation
    var shakeStyle = document.createElement('style');
    shakeStyle.textContent = '@keyframes shake { 0%, 100% { transform: translateX(0); } 10%, 30%, 50%, 70%, 90% { transform: translateX(-2px); } 20%, 40%, 60%, 80% { transform: translateX(2px); } }';
    document.head.appendChild(shakeStyle);
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  Serial.println(">> Serving main page...");
  server.send(200, "text/html", htmlPage);
}

void rattleTail() {
  rattlingActive = true;
  rattlingStart = millis();
  locomotionEnabled = false;
  testingActive = false;
  bodyHasCurled = false;
  Serial.println("*** RATTLE ATTACK ACTIVATED! ***");
}

void updateTailRattle() {
  if (!rattlingActive) return;
  
  unsigned long elapsed = millis() - rattlingStart;
  if (elapsed > RATTLING_DURATION) {
    rattlingActive = false;
    bodyHasCurled = false;
    for (int i = 0; i < NUM_SERVOS; i++) {
      servos[i].write(centerPosition);
    }
    Serial.println("*** Rattle complete ***");
    return;
  }
  
  float t = elapsed / 1000.0f;
  
  if (!bodyHasCurled && elapsed < 800) {
    for (int i = 0; i < 5; i++) {
      if (servoLayout[i] == 1) {
        float curlProgress = elapsed / 800.0f;
        float curlAngle = centerPosition + CURL_AMPLITUDE * sin(curlProgress * PI) * 0.8;
        servos[i].write(curlAngle);
      }
    }
  } else if (elapsed >= 800 && !bodyHasCurled) {
    bodyHasCurled = true;
    for (int i = 0; i < 5; i++) {
      if (servoLayout[i] == 1) {
        float finalCurlAngle = centerPosition + CURL_AMPLITUDE * 0.6;
        servos[i].write(finalCurlAngle);
      }
    }
    Serial.println("*** Body curled - tail rattling ***");
  }
  
  float liftAngle = centerPosition - 45;
  servos[tailLiftIdx].write(liftAngle);
  
  float rattle1 = centerPosition + RATTLING_AMPLITUDE * sin(2 * PI * RATTLING_FREQ * t);
  float rattle2 = centerPosition + RATTLING_AMPLITUDE * sin(2 * PI * RATTLING_FREQ * t + PI);
  
  servos[tailRattle1Idx].write(rattle1);
  servos[tailRattle2Idx].write(rattle2);
}

void centerAllServos() {
  locomotionEnabled = false;
  testingActive = false;
  rattlingActive = false;
  bodyHasCurled = false;
  Serial.println(">> Centering all servos...");
  
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].write(centerPosition);
  }
  Serial.println(">> All servos centered!");
}

void handleServoTest() {
  if (millis() - lastTestMillis >= 500) {
    lastTestMillis = millis();
    
    switch (testPosition) {
      case 0: servos[testServoIndex].write(0); break;
      case 1: servos[testServoIndex].write(90); break;
      case 2: servos[testServoIndex].write(180); break;
      case 3: servos[testServoIndex].write(90); break;
    }

    if (++testPosition > 3) {
      testPosition = 0;
      if (++testServoIndex >= NUM_SERVOS) {
        testingActive = false;
        testServoIndex = 0;
        centerAllServos();
        Serial.println(">> Test complete");
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
      servos[i].write(angle);
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
      servos[i].write(angle);
      horizontalIndex++;
    } else {
      float basePhase = verticalIndex * phaseOffset + verticalPhaseOffset;
      float phase = forwardDirection ? 
        radians((NUM_SERVOS - NUM_HORIZONTAL - 1 - verticalIndex) * phaseOffset + verticalPhaseOffset) : 
        radians(basePhase);
      
      float angle = centerPosition + verticalAmplitude * sin(timeScale + phase);
      servos[i].write(angle);
      verticalIndex++;
    }
  }
}

void handleSetParameters() {
  String response = "";
  
  Serial.println("=== POST Request ===");
  for (int i = 0; i < server.args(); i++) {
    Serial.println(server.argName(i) + " = " + server.arg(i));
  }
  
  if (server.hasArg("test")) {
    response = "🤖 Connection OK!";
    Serial.println(">> Connection test");
  }
  
  if (server.hasArg("mode")) {
    movementMode = server.arg("mode").toInt();
    response = "🎯 Mode: " + String(movementMode == 0 ? "Lateral" : "Sidewinding");
    Serial.println(">> Mode: " + String(movementMode));
  }
  
  if (server.hasArg("action")) {
    String action = server.arg("action");
    Serial.println(">> Action: " + action);
    
    if (action == "center") {
      centerAllServos();
      response = "🎯 Centered!";
    } else if (action == "forward") {
      locomotionEnabled = true;
      forwardDirection = true;
      rattlingActive = false;
      response = "⬆️ Forward!";
    } else if (action == "backward") {
      locomotionEnabled = true;
      forwardDirection = false;
      rattlingActive = false;
      response = "⬇️ Backward!";
    } else if (action == "stop") {
      locomotionEnabled = false;
      rattlingActive = false;
      centerAllServos();
      response = "⏹️ Stopped!";
    } else if (action == "test") {
      testingActive = true;
      rattlingActive = false;
      testServoIndex = 0;
      testPosition = 0;
      response = "🔧 Testing!";
    } else if (action == "rattle") {
      rattleTail();
      response = "🐍 RATTLE ATTACK! 🐍";
    }
  }

  if (server.hasArg("amplitude")) {
    amplitude = constrain(server.arg("amplitude").toFloat(), 10.0, 45.0);
    response += " Amp:" + String(amplitude);
  }
  if (server.hasArg("frequency")) {
    frequency = constrain(server.arg("frequency").toFloat(), 0.1, 2.0);
    response += " Freq:" + String(frequency);
  }
  if (server.hasArg("phase")) {
    phaseOffset = constrain(server.arg("phase").toFloat(), 30.0, 90.0);
    response += " Phase:" + String(phaseOffset);
  }
  
  if (response == "") response = "❓ Unknown";
  
  Serial.println(">> Response: " + response);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", response);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("🐍 SNAKE ROBOT STARTING 🐍");
  
  // Initialize servos
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(centerPosition);
    Serial.printf("Servo %d: Pin %d\n", i+1, servoPins[i]);
  }

  // Start WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32_Snake", "12345678");
  delay(2000);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.println("WiFi AP Started!");
  Serial.print("IP: ");
  Serial.println(IP);
  Serial.println("Network: ESP32_Snake");
  Serial.println("Password: 12345678");

  // Start server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/", HTTP_POST, handleSetParameters);
  server.begin();
  Serial.println("Server started!");
  Serial.println("🤖 READY! 🤖");
}

void loop() {
  server.handleClient();
  
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
}