#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <math.h>

// Network credentials
WebServer server(80);

// Servo configuration
const int NUM_SERVOS = 10;
const int servoLayout[NUM_SERVOS] = {1, 0, 1, 1, 0, 1, 1, 0, 1, 1}; // 1 = horizontal, 0 = vertical
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
int interval = 20; // 50Hz update rate

// HTML content with enhanced UI
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Snake Control</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background-color: #f2f2f2;
      padding: 20px;
    }
    h2 {
      color: #333;
    }
    button, input {
      font-size: 20px;
      padding: 10px;
      margin: 10px;
      border-radius: 5px;
      border: none;
      cursor: pointer;
    }
    button {
      background-color: #007BFF;
      color: white;
    }
    button:hover {
      background-color: #0056b3;
    }
    input {
      width: 80px;
      text-align: center;
    }
    .response {
      margin-top: 20px;
      font-size: 18px;
      color: green;
    }
    .direction-controls {
      margin: 20px 0;
    }
  </style>
</head>
<body>
  <h2>Snake Locomotion Control</h2>
  <div class="direction-controls">
    <button onclick="sendAction('forward')">Forward</button>
    <button onclick="sendAction('backward')">Backward</button>
    <button onclick="sendAction('stop')">Stop</button>
    <button onclick="sendAction('test')">Test Servos</button>
    <button onclick="sendAction('center')" style="background-color: #28a745;">Center All Servos</button>
  </div>
  <div>
    <label for="amplitude">Amplitude:</label>
    <input type="number" id="amplitude" min="10" max="45" step="1" value="30">
    <label for="frequency">Frequency:</label>
    <input type="number" id="frequency" min="0.1" max="2.0" step="0.1" value="1.0">
    <button onclick="setParameters()">Set Parameters</button>
  </div>
  <div class="response" id="response"></div>
  <script>
    function sendAction(action) {
      fetch('/', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'action=' + action
      })
      .then(response => response.text())
      .then(data => {
        document.getElementById('response').innerHTML = data;
      });
    }
    function setParameters() {
      const amplitude = document.getElementById('amplitude').value;
      const frequency = document.getElementById('frequency').value;
      fetch('/', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'amplitude=' + amplitude + '&frequency=' + frequency
      })
      .then(response => response.text())
      .then(data => {
        document.getElementById('response').innerHTML = data;
      });
    }
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void centerAllServos() {
  locomotionEnabled = false;
  Serial.println("Centering all servos...");
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].write(centerPosition);
    Serial.printf("Servo %d centered to %.1f degrees\n", i, centerPosition);
    delay(100); // Small delay between servos
  }
  delay(500); // Allow servos to reach position
}

void handleSetParameters() {
  String response;
  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action == "center") {
      centerAllServos();
      response = "All servos centered to " + String(centerPosition) + " degrees.";
    } else if (action == "forward") {
      locomotionEnabled = true;
      forwardDirection = true;
      response = "Moving forward.";
    } else if (action == "backward") {
      locomotionEnabled = true;
      forwardDirection = false;
      response = "Moving backward.";
    } else if (action == "stop") {
      locomotionEnabled = false;
      response = "Stopped.";
      // Return all servos to center
      for (int i = 0; i < NUM_SERVOS; i++) {
        servos[i].write(centerPosition);
      }
    } else if (action == "test") {
      response = "Testing servos...";
      testServos();
    }
  }
  if (server.hasArg("amplitude")) {
    amplitude = constrain(server.arg("amplitude").toFloat(), 10.0, 45.0);
    response += " Amplitude set to " + String(amplitude) + ".";
  }
  if (server.hasArg("frequency")) {
    frequency = constrain(server.arg("frequency").toFloat(), 0.1, 2.0);
    response += " Frequency set to " + String(frequency) + ".";
  }
  server.send(200, "text/plain", response);
}

void updateServos() {
    float timeScale = (millis() * 0.001) * frequency * 2.0 * PI; // Convert to seconds and scale
    int horizontalServoIndex = 0; // Keep track of horizontal servos only
    
    for (int i = 0; i < NUM_SERVOS; i++) {
        if (servoLayout[i] == 1) { // Horizontal servo
            // Calculate phase based on direction
            float phase;
            if (forwardDirection) {
                phase = radians(horizontalServoIndex * phaseOffset);
            } else {
                phase = radians((6 - horizontalServoIndex) * phaseOffset); // 6 is number of horizontal servos - 1
            }
            
            // Calculate angle using sine wave
            float angle = amplitude * sin(timeScale + phase);
            servos[i].write(centerPosition + angle);
            
            horizontalServoIndex++; // Increment only for horizontal servos
        } else {
            // Vertical servo - maintain center position
            servos[i].write(centerPosition);
        }
    }
}

void testServos() {
  locomotionEnabled = false;
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].write(0);
    delay(500);
    servos[i].write(90);
    delay(500);
    servos[i].write(180);
    delay(500);
    servos[i].write(90);
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize servos
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(centerPosition);
  }
  delay(1000); // Allow servos to reach center position

  // Setup WiFi Access Point
  WiFi.softAP("ESP32_Snake", "12345678");
  Serial.println("Access Point started");
  Serial.println("IP address: " + WiFi.softAPIP().toString());

  // Setup server handlers
  server.on("/", HTTP_GET, handleRoot);
  server.on("/", HTTP_POST, handleSetParameters);
  server.begin();
}

void loop() {
  server.handleClient();

  if (locomotionEnabled && millis() - lastUpdate >= interval) {
    updateServos();
    lastUpdate = millis();
  }
}