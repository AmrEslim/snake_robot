#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <math.h>

WebServer server(80);

// Servo configuration
const int NUM_SERVOS = 10;
const int NUM_HORIZONTAL = 7; // Calculated from servoLayout (1,0,1,1,0,1,1,0,1,1)
const int servoLayout[NUM_SERVOS] = {1, 0, 1, 1, 0, 1, 1, 0, 1, 1};
const int servoPins[NUM_SERVOS] = {2, 4, 16, 17, 5, 18, 19, 21, 22, 23};

Servo servos[NUM_SERVOS];

// Locomotion parameters
bool locomotionEnabled = false;
bool forwardDirection = true;
float amplitude = 30.0;
float frequency = 1.0;
float phaseOffset = 60.0;
float centerPosition = 90.0;
unsigned long lastUpdate = 0;
const int interval = 20; // 50Hz update rate

// Servo test variables
bool testingActive = false;
int testServoIndex = 0;
int testPosition = 0;
unsigned long lastTestMillis = 0;

// HTML content with phase control
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Snake Control</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; padding: 20px; background-color: #f2f2f2; }
    h2 { color: #333; }
    button, input { font-size: 20px; padding: 10px; margin: 10px; border-radius: 5px; border: none; cursor: pointer; }
    button { background-color: #007BFF; color: white; }
    button:hover { background-color: #0056b3; }
    input { width: 80px; text-align: center; }
    .response { margin-top: 20px; font-size: 18px; color: green; }
    .direction-controls { margin: 20px 0; }
  </style>
</head>
<body>
  <h2>Snake Locomotion Control</h2>
  <div class="direction-controls">
    <button onclick="sendAction('forward')">Forward</button>
    <button onclick="sendAction('backward')">Backward</button>
    <button onclick="sendAction('stop')">Stop</button>
    <button onclick="sendAction('test')">Test Servos</button>
    <button onclick="sendAction('center')" style="background-color: #28a745;">Center All</button>
  </div>
  <div>
    <label for="amplitude">Amplitude:</label>
    <input type="number" id="amplitude" min="10" max="45" step="1" value="30">
    <label for="frequency">Frequency:</label>
    <input type="number" id="frequency" min="0.1" max="2.0" step="0.1" value="1.0">
    <label for="phase">Phase:</label>
    <input type="number" id="phase" min="30" max="90" step="5" value="60">
    <button onclick="setParameters()">Update</button>
  </div>
  <div class="response" id="response"></div>
  <script>
    function sendAction(action) {
      fetch('/', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: 'action=' + action })
        .then(r => r.text()).then(d => document.getElementById('response').innerHTML = d);
    }
    function setParameters() {
      const params = new URLSearchParams();
      params.append('amplitude', document.getElementById('amplitude').value);
      params.append('frequency', document.getElementById('frequency').value);
      params.append('phase', document.getElementById('phase').value);
      fetch('/', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: params })
        .then(r => r.text()).then(d => document.getElementById('response').innerHTML = d);
    }
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void handleSetParameters() {
  String response;
  
  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action == "center") {
      centerAllServos();
      response = "All servos centered";
    } else if (action == "forward") {
      locomotionEnabled = true;
      forwardDirection = true;
      response = "Forward motion enabled";
    } else if (action == "backward") {
      locomotionEnabled = true;
      forwardDirection = false;
      response = "Backward motion enabled";
    } else if (action == "stop") {
      locomotionEnabled = false;
      centerAllServos();
      response = "Stopped and centered";
    } else if (action == "test") {
      testingActive = true;
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

void centerAllServos() {
  locomotionEnabled = false;
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].write(centerPosition);
  }
}

void updateServos() {
  float timeScale = millis() * 0.001 * frequency * 2 * PI;
  int horizontalIndex = 0;

  for (int i = 0; i < NUM_SERVOS; i++) {
    if (servoLayout[i] == 1) {
      float phase;
      if (forwardDirection) {
        phase = radians(horizontalIndex * phaseOffset);
      } else {
        phase = radians((NUM_HORIZONTAL - 1 - horizontalIndex) * phaseOffset);
      }
      
      float angle = centerPosition + amplitude * sin(timeScale + phase);
      servos[i].write(angle);
      horizontalIndex++;
    } else {
      servos[i].write(centerPosition);
    }
  }
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
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize servos
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(centerPosition);
  }
  delay(1000);

  // Start WiFi AP
  WiFi.softAP("ESP32_Snake", "12345678");
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  // Configure server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/", HTTP_POST, handleSetParameters);
  server.begin();
}

void loop() {
  server.handleClient();

  if (testingActive) {
    handleServoTest();
  }
  else if (locomotionEnabled && (millis() - lastUpdate >= interval)) {
    updateServos();
    lastUpdate = millis();
  }
}