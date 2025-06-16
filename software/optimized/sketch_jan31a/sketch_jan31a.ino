#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <math.h>

WebServer server(80);

// Servo configuration
const int NUM_SERVOS = 10;
const int NUM_HORIZONTAL = 7;  // Number of horizontal servos
const int servoLayout[NUM_SERVOS] = {1, 0, 1, 1, 0, 1, 1, 0, 1, 1}; // 1=horizontal, 0=vertical
const int servoPins[NUM_SERVOS] = {23, 22, 2, 4, 16, 17, 5, 18, 19, 21};

// Rattling configuration
const int tailLiftIdx = 7;   // 8th motor (index 7) - lifts tail up
const int tailRattle1Idx = 8; // 9th motor (index 8) - first rattle motor
const int tailRattle2Idx = 9; // 10th motor (index 9) - second rattle motor

bool rattlingActive = false;
unsigned long rattlingStart = 0;
const unsigned long RATTLING_DURATION = 5000; // 5 seconds
const float RATTLING_FREQ = 30.0; // High frequency for intense rattle
const float RATTLING_AMPLITUDE = 45.0;
const float CURL_AMPLITUDE = 30.0; // For body curling effect
bool bodyHasCurled = false; // Track if body has done its curl

Servo servos[NUM_SERVOS];

// Locomotion parameters
bool locomotionEnabled = false;
bool forwardDirection = true;
int movementMode = 0;  // 0: Lateral undulation, 1: Sidewinding
float amplitude = 30.0;
float frequency = 1.0;
float phaseOffset = 60.0;
float verticalAmplitude = 20.0;  // For sidewinding
float verticalPhaseOffset = 90.0; // 90 degrees offset for sidewinding
float centerPosition = 90.0;
unsigned long lastUpdate = 0;
const int interval = 20;  // 50Hz update rate

// Servo test variables
bool testingActive = false;
int testServoIndex = 0;
int testPosition = 0;
unsigned long lastTestMillis = 0;

// HTML Interface - Simplified without logging features
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Snake Robot Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { 
      font-family: Arial, sans-serif; 
      text-align: center; 
      padding: 20px; 
      background-color: #f2f2f2; 
    }
    .container {
      max-width: 800px;
      margin: 0 auto;
    }
    h2 { 
      color: #333; 
      margin-bottom: 20px;
    }
    button, input, select { 
      font-size: 18px; 
      padding: 10px; 
      margin: 8px; 
      border-radius: 5px; 
      border: 1px solid #ccc;
      cursor: pointer; 
    }
    button { 
      background-color: #007BFF; 
      color: white; 
      border: none;
      min-width: 120px;
    }
    button:hover { 
      background-color: #0056b3; 
    }
    button.danger {
      background-color: #dc3545;
    }
    button.success {
      background-color: #28a745;
    }
    button.warning {
      background-color: #ffc107;
      color: #212529;
    }
    button.rattle {
      background-color: #6f42c1;
      color: white;
      font-weight: bold;
      font-size: 20px;
    }
    button.rattle:hover {
      background-color: #5a2d91;
    }
    input, select { 
      width: 120px; 
      text-align: center;
    }
    .control-group {
      margin: 20px 0;
      padding: 15px;
      background-color: white;
      border-radius: 8px;
      box-shadow: 0 2px 4px rgba(0,0,0,0.1);
    }
    .response { 
      margin-top: 20px; 
      font-size: 18px; 
      color: #28a745;
      padding: 10px;
      background-color: #f8f9fa;
      border-radius: 5px;
    }
    label {
      display: inline-block;
      width: 100px;
      text-align: right;
      margin-right: 10px;
    }
    @media (max-width: 600px) {
      button, input, select {
        width: 100%;
        margin: 5px 0;
      }
      label {
        width: 100%;
        text-align: left;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <h2>🐍 Snake Robot Control</h2>
    
    <div class="control-group">
      <select id="mode" onchange="sendMode()" style="width: 200px;">
        <option value="0">Lateral Undulation</option>
        <option value="1">Sidewinding</option>
      </select>
    </div>
    
    <div class="control-group">
      <button onclick="sendAction('forward')">Forward</button>
      <button onclick="sendAction('backward')">Backward</button>
      <button onclick="sendAction('stop')" class="danger">Stop</button>
      <button onclick="sendAction('center')" class="success">Center All</button>
      <button onclick="sendAction('test')">Test Servos</button>
      <button onclick="sendAction('rattle')" class="rattle">🐍 RATTLE TAIL 🐍</button>
    </div>

    <div class="control-group">
      <div>
        <label for="amplitude">Amplitude:</label>
        <input type="number" id="amplitude" min="10" max="45" step="1" value="30">
      </div>
      <div>
        <label for="frequency">Frequency:</label>
        <input type="number" id="frequency" min="0.1" max="2.0" step="0.1" value="1.0">
      </div>
      <div>
        <label for="phase">Phase:</label>
        <input type="number" id="phase" min="30" max="90" step="5" value="60">
      </div>
      <button onclick="setParameters()">Update Parameters</button>
    </div>

    <div class="response" id="response"></div>
  </div>

  <script>
    function sendMode() {
      const mode = document.getElementById('mode').value;
      sendRequest('mode=' + mode);
    }

    function sendAction(action) {
      sendRequest('action=' + action);
    }

    function setParameters() {
      const params = new URLSearchParams();
      params.append('amplitude', document.getElementById('amplitude').value);
      params.append('frequency', document.getElementById('frequency').value);
      params.append('phase', document.getElementById('phase').value);
      sendRequest(params.toString());
    }

    function sendRequest(data) {
      fetch('/', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: data
      })
      .then(response => response.text())
      .then(data => {
        document.getElementById('response').innerHTML = data;
        setTimeout(() => document.getElementById('response').innerHTML = '', 3000);
      })
      .catch(error => {
        document.getElementById('response').innerHTML = 'Error: ' + error;
      });
    }
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void rattleTail() {
  rattlingActive = true;
  rattlingStart = millis();
  locomotionEnabled = false;
  testingActive = false;
  bodyHasCurled = false; // Reset curl state
  Serial.println("🐍 Rattling activated!");
}

void updateTailRattle() {
  if (!rattlingActive) return;
  
  unsigned long elapsed = millis() - rattlingStart;
  if (elapsed > RATTLING_DURATION) {
    rattlingActive = false;
    bodyHasCurled = false;
    // Return all servos to center positions
    for (int i = 0; i < NUM_SERVOS; i++) {
      servos[i].write(centerPosition);
    }
    Serial.println("Rattle sequence complete - all servos centered");
    return;
  }
  
  float t = elapsed / 1000.0f;
  
  // Body curl once at the beginning and stay static
  if (!bodyHasCurled && elapsed < 800) { // Curl for first 0.8 seconds
    for (int i = 0; i < 5; i++) { // First 5 servos
      if (servoLayout[i] == 1) { // Only horizontal servos
        float curlProgress = elapsed / 800.0f; // 0 to 1 over 0.8 seconds
        float curlAngle = centerPosition + CURL_AMPLITUDE * sin(curlProgress * PI) * 0.8; // Smooth curl
        servos[i].write(curlAngle);
      }
    }
  } else if (elapsed >= 800 && !bodyHasCurled) {
    // Fix body in curled position
    bodyHasCurled = true;
    for (int i = 0; i < 5; i++) {
      if (servoLayout[i] == 1) {
        float finalCurlAngle = centerPosition + CURL_AMPLITUDE * 0.6; // Static curl position
        servos[i].write(finalCurlAngle);
      }
    }
    Serial.println("Body curled and locked in position");
  }
  
  // Tail operations - continuous throughout the duration
  // Lift tail up (8th motor - index 7)
  float liftAngle = centerPosition - 45; // Lift tail up higher
  servos[tailLiftIdx].write(liftAngle);
  
  // Intense alternating rattle pattern for 9th and 10th motors
  float highFreqRattle1 = centerPosition + RATTLING_AMPLITUDE * sin(2 * PI * RATTLING_FREQ * t);
  float highFreqRattle2 = centerPosition + RATTLING_AMPLITUDE * sin(2 * PI * RATTLING_FREQ * t + PI); // 180° out of phase
  
  servos[tailRattle1Idx].write(highFreqRattle1);
  servos[tailRattle2Idx].write(highFreqRattle2);
}

void centerAllServos() {
  locomotionEnabled = false;
  testingActive = false;
  rattlingActive = false;
  bodyHasCurled = false;
  Serial.println("Centering all servos...");
  
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].write(centerPosition);
  }
  Serial.println("All servos centered");
}

void handleServoTest() {
  if (millis() - lastTestMillis >= 500) {
    lastTestMillis = millis();
    
    switch (testPosition) {
      case 0:
        servos[testServoIndex].write(0);
        break;
      case 1:
        servos[testServoIndex].write(90);
        break;
      case 2:
        servos[testServoIndex].write(180);
        break;
      case 3:
        servos[testServoIndex].write(90);
        break;
    }

    if (++testPosition > 3) {
      testPosition = 0;
      if (++testServoIndex >= NUM_SERVOS) {
        testingActive = false;
        testServoIndex = 0;
        centerAllServos();
      }
    }
  }
}

void updateServosLateralUndulation() {
  float timeScale = millis() * 0.001 * frequency * 2 * PI;
  int horizontalIndex = 0;
  
  for (int i = 0; i < NUM_SERVOS; i++) {
    if (servoLayout[i] == 1) {  // Horizontal servos
      float segmentPhase;
      
      if (forwardDirection) {
        segmentPhase = (NUM_HORIZONTAL - 1 - horizontalIndex) * radians(phaseOffset);
      } else {
        segmentPhase = horizontalIndex * radians(phaseOffset);
      }
      
      float angle = centerPosition + amplitude * sin(timeScale + segmentPhase);
      servos[i].write(angle);
      horizontalIndex++;
    } else {  // Vertical servos remain centered
      servos[i].write(centerPosition);
    }
  }
}

void updateServosSidewinding() {
  float timeScale = millis() * 0.001 * frequency * 2 * PI;
  int horizontalIndex = 0;
  int verticalIndex = 0;

  for (int i = 0; i < NUM_SERVOS; i++) {
    if (servoLayout[i] == 1) {  // Horizontal servos
      float phase = forwardDirection ? 
        radians((NUM_HORIZONTAL - 1 - horizontalIndex) * phaseOffset) : 
        radians(horizontalIndex * phaseOffset);
      
      float angle = centerPosition + amplitude * sin(timeScale + phase);
      servos[i].write(angle);
      horizontalIndex++;
    } else {  // Vertical servos
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
  String response;
  
  if (server.hasArg("mode")) {
    movementMode = server.arg("mode").toInt();
    response = "Mode set to: " + String(movementMode == 0 ? "Lateral Undulation" : "Sidewinding");
  }
  
  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action == "center") {
      centerAllServos();
      response = "All servos centered";
    } else if (action == "forward") {
      locomotionEnabled = true;
      forwardDirection = true;
      rattlingActive = false;
      response = "Forward motion enabled";
    } else if (action == "rattle") {
      rattleTail();
      response = "🐍 RATTLING INITIATED! Body will curl once, tail will rattle intensely for 5 seconds! 🐍";
    } else if (action == "backward") {
      locomotionEnabled = true;
      forwardDirection = false;
      rattlingActive = false;
      response = "Backward motion enabled";
    } else if (action == "stop") {
      locomotionEnabled = false;
      rattlingActive = false;
      centerAllServos();
      response = "Stopped and centered";
    } else if (action == "test") {
      testingActive = true;
      rattlingActive = false;
      testServoIndex = 0;
      testPosition = 0;
      response = "Starting servo test...";
    }
  }

  if (server.hasArg("amplitude")) {
    amplitude = constrain(server.arg("amplitude").toFloat(), 10.0, 45.0);
    response += " Amplitude: " + String(amplitude);
  }
  if (server.hasArg("frequency")) {
    frequency = constrain(server.arg("frequency").toFloat(), 0.1, 2.0);
    response += " Frequency: " + String(frequency);
  }
  if (server.hasArg("phase")) {
    phaseOffset = constrain(server.arg("phase").toFloat(), 30.0, 90.0);
    response += " Phase: " + String(phaseOffset);
  }
  
  server.send(200, "text/plain", response);
}

void setup() {
  Serial.begin(115200);
  
  // Initialize all servos
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(centerPosition);
  }

  // Setup WiFi Access Point
  WiFi.softAP("ESP32_Snake", "12345678");
  Serial.println("🐍 Snake Robot Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Setup server handlers
  server.on("/", HTTP_GET, handleRoot);
  server.on("/", HTTP_POST, handleSetParameters);
  server.begin();
  Serial.println("HTTP server started - Ready to rattle! 🐍");
}

void loop() {
  server.handleClient();
  
  // Handle rattling with highest priority
  if (rattlingActive) {
    updateTailRattle();
  }
  // Handle servo operations only if not rattling
  else if (testingActive) {
    handleServoTest();
  }
  else if (locomotionEnabled && (millis() - lastUpdate >= interval)) {
    if (movementMode == 0) {
      updateServosLateralUndulation();
    } else {
      updateServosSidewinding();
    }
    lastUpdate = millis();
  }
}