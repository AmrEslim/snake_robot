#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <math.h>

// Network credentials
WebServer server(80);

Servo servos[10];
int servoPins[10] = {2, 4, 16, 17, 5, 18, 19, 21, 22, 23};

// Locomotion parameters
bool locomotionEnabled = false;
float amplitude = 30.0;
float frequency = 1.0;
unsigned long lastUpdate = 0;
int interval = 50;

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
  </style>
</head>
<body>
  <h2>Snake Locomotion Control</h2>
  <div>
    <button onclick="sendAction('start')">Start Locomotion</button>
    <button onclick="sendAction('stop')">Stop Locomotion</button>
    <button onclick="sendAction('test')">Test Servos</button>
  </div>
  <div>
    <label for="amplitude">Amplitude:</label>
    <input type="number" id="amplitude" min="0" max="45" step="1" value="30">
    <label for="frequency">Frequency:</label>
    <input type="number" id="frequency" min="0.1" max="5.0" step="0.1" value="1.0">
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

// Handle the HTML page
void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

// Handle form submissions dynamically
void handleSetParameters() {
  String response;
  if (server.hasArg("action")) {
    String action = server.arg("action");
    if (action == "start") {
      locomotionEnabled = true;
      response = "Locomotion started.";
    } else if (action == "stop") {
      locomotionEnabled = false;
      response = "Locomotion stopped.";
    } else if (action == "test") {
      response = "Testing servos...";
      testServos();
    }
  }
  if (server.hasArg("amplitude")) {
    amplitude = server.arg("amplitude").toFloat();
    response += " Amplitude set to " + String(amplitude) + ".";
  }
  if (server.hasArg("frequency")) {
    frequency = server.arg("frequency").toFloat();
    response += " Frequency set to " + String(frequency) + ".";
  }
  server.send(200, "text/plain", response);
}

// Update servo positions for locomotion
void updateServos() {
  float phaseStep = 2 * PI / 10;
  float time = millis() / 1000.0;
  for (int i = 0; i < 10; i++) {
    float angle = 90 + amplitude * sin(2 * PI * frequency * time + phaseStep * i);
    servos[i].write(angle);
  }
}

// Test servos one by one
void testServos() {
  for (int i = 0; i < 10; i++) {
    servos[i].write(0);
    delay(500);
    servos[i].write(90);
    delay(500);
    servos[i].write(180);
    delay(500);
    servos[i].write(90);
  }
}

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 10; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(90);
  }

  WiFi.softAP("ESP32_Snake", "12345678");
  Serial.println("Access Point started");
  Serial.println("IP address: " + WiFi.softAPIP().toString());

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
