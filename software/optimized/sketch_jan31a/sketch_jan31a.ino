#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <math.h>
#include <ACS712.h>
#include <SPIFFS.h>

WebServer server(80);

// Servo configuration
const int analogPin = 34;
const int NUM_SERVOS = 10;
const int NUM_HORIZONTAL = 7;  // Number of horizontal servos
const int servoLayout[NUM_SERVOS] = {1, 0, 1, 1, 0, 1, 1, 0, 1, 1}; // 1=horizontal, 0=vertical
const int servoPins[NUM_SERVOS] = {23, 22, 2, 4, 16, 17, 5, 18, 19, 21};

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

// Logging variables
bool loggingEnabled = false;
unsigned long loggingStartTime = 0;
unsigned long lastLogTime = 0;
const unsigned long LOGGING_DURATION = 10000; // 10 seconds in milliseconds
const unsigned long LOGGING_INTERVAL = 100; // Log every 100ms (10Hz)

//initialize the sensor
ACS712 sensor(analogPin, 3.3, 4095, 100.0); // pin, voltage, ADC max, mV/A sensitivity

// File for logging data
File logFile;

// HTML Interface
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
    <h2>Snake Robot Control</h2>
    
    <div class="control-group">
      <select id="mode" onchange="sendMode()" style="width: 200px;">
        <option value="0">Lateral Undulation</option>
        <option value="1">Sidewinding</option>
      </select>
    </div>
    <div class="control-group">
      <h3>Current Sensor Data</h3>
      <p>Current Reading: <span id="currentReading">0.00</span> A</p>
      <p>Logging Status: <span id="loggingStatus">Inactive</span></p>
      <div style="display: flex; justify-content: center; gap: 10px;">
        <button onclick="downloadData()">Download Data</button>
        <button onclick="clearLogData()" class="danger">Clear Log</button>
        <button onclick="sendAction('logIdle')" class="warning">Log Idle (10s)</button>
      </div>
    </div>
    <div class="control-group">
      <button onclick="sendAction('forward')">Forward</button>
      <button onclick="sendAction('backward')">Backward</button>
      <button onclick="sendAction('stop')" class="danger">Stop</button>
      <button onclick="sendAction('center')" class="success">Center All</button>
      <button onclick="sendAction('test')">Test Servos</button>
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
    function fetchCurrentData() {
      fetch('/current')
        .then(response => response.json())
        .then(data => {
          document.getElementById('currentReading').innerText = data.current.toFixed(2);
          document.getElementById('loggingStatus').innerText = data.logging ? 'Active' : 'Inactive';
        })
        .catch(error => console.error('Error fetching current data:', error));
    }

    // Function to download data
    function downloadData() {
      window.location.href = '/download';
    }
    
    // Function to clear log data
    function clearLogData() {
      if (confirm('Are you sure you want to clear all logged data?')) {
        fetch('/clearlog')
          .then(response => response.text())
          .then(data => {
            document.getElementById('response').innerHTML = data;
            setTimeout(() => document.getElementById('response').innerHTML = '', 3000);
          })
          .catch(error => {
            document.getElementById('response').innerHTML = 'Error: ' + error;
          });
      }
    }

    // Periodically fetch current data every 30 milli seconds
    setInterval(fetchCurrentData, 30);
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void logCurrentData(float current) {
  if (!loggingEnabled) return;
  
  File file = SPIFFS.open("/current_log.txt", FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  
  // Calculate time in seconds since logging started (with millisecond precision)
  float elapsedTime = (millis() - loggingStartTime) / 1000.0;
  
  // Write data in CSV format: "seconds,current"
  file.printf("%.3f,%.3f\n", elapsedTime, current);
  file.close();
}

void handleCurrentData() {
  float current = 0.0;
  // Take multiple readings for better accuracy
  for(int i = 0; i < 10; i++) {
    current += abs(sensor.mA_DC());
    delay(1);
  }
  current = (current / 10.0) / 1000.0;  // Convert to Amps
  
  String jsonResponse = "{\"current\": " + String(current, 3) + ", \"logging\": " + (loggingEnabled ? "true" : "false") + "}";
  server.send(200, "application/json", jsonResponse);
  
  // Only print to serial occasionally to avoid slowing things down
  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime > 500) { // Print every 500ms
    Serial.print("Current reading: ");
    Serial.println(current, 3);
    if (loggingEnabled) {
      Serial.print("Logging: ");
      Serial.print((millis() - loggingStartTime) / 1000.0);
      Serial.println(" seconds elapsed");
    }
    lastPrintTime = millis();
  }
}

void handleDownload() {
  if(!SPIFFS.exists("/current_log.txt")) {
    server.send(404, "text/plain", "No data log found");
    return;
  }

  File file = SPIFFS.open("/current_log.txt", "r");
  if(!file) {
    server.send(500, "text/plain", "Failed to open file");
    return;
  }

  // Set up headers for file download
  server.sendHeader("Content-Type", "text/csv");
  server.sendHeader("Content-Disposition", "attachment; filename=current_log.csv");
  server.sendHeader("Connection", "close");
  
  // Add CSV header row for better readability
  String csvHeader = "Time(s),Current(A)\r\n";
  server.client().write((const uint8_t*)csvHeader.c_str(), csvHeader.length());
  
  // Stream file to client
  size_t fileSize = file.size();
  server.setContentLength(fileSize + csvHeader.length());
  
  // Send file in chunks
  size_t chunkSize = 1024;
  uint8_t buf[1024];
  while(file.available()) {
    size_t len = file.read(buf, chunkSize);
    server.client().write(buf, len);
  }
  
  file.close();
}

// Handle clearing the log file
void handleClearLog() {
  if(SPIFFS.exists("/current_log.txt")) {
    if(SPIFFS.remove("/current_log.txt")) {
      server.send(200, "text/plain", "Log file cleared successfully");
      Serial.println("Log file cleared by user request");
    } else {
      server.send(500, "text/plain", "Failed to clear log file");
      Serial.println("Failed to clear log file");
    }
  } else {
    server.send(200, "text/plain", "No log file exists");
    Serial.println("Clear log requested but no file exists");
  }
}

void startLogging() {
  // Clear existing log if it exists
  if(SPIFFS.exists("/current_log.txt")) {
    SPIFFS.remove("/current_log.txt");
  }
  
  loggingEnabled = true;
  loggingStartTime = millis();
  lastLogTime = millis(); // Reset the last log time
  Serial.println("Logging started for 10 seconds at 10Hz");
}

void centerAllServos() {
  locomotionEnabled = false;
  testingActive = false;
  Serial.println("Centering all servos...");
  
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].write(centerPosition);
    Serial.printf("Servo %d centered to %.1f degrees\n", i, centerPosition);
    delay(100);
  }
  delay(500);
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
        // For forward motion, we need to reverse the phase progression
        segmentPhase = (NUM_HORIZONTAL - 1 - horizontalIndex) * radians(phaseOffset);
      } else {
        // For backward motion, use the original phase progression
        segmentPhase = horizontalIndex * radians(phaseOffset);
      }
      
      // Calculate servo angle with full amplitude
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
      // Adjust the vertical wave phase based on direction as well
      float basePhase = verticalIndex * phaseOffset + verticalPhaseOffset;
      // You might need to experiment with this part to get the right behavior
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
      startLogging(); // Start logging when forward movement begins
      response = "Forward motion enabled (logging for 10 seconds)";
    } else if (action == "backward") {
      locomotionEnabled = true;
      forwardDirection = false;
      startLogging(); // Start logging when backward movement begins
      response = "Backward motion enabled (logging for 10 seconds)";
    } else if (action == "stop") {
      locomotionEnabled = false;
      loggingEnabled = false; // Stop logging when movement stops
      centerAllServos();
      response = "Stopped and centered";
    } else if (action == "test") {
      testingActive = true;
      testServoIndex = 0;
      testPosition = 0;
      response = "Starting servo test...";
    } else if (action == "logIdle") {
      // Make sure the robot is not moving
      locomotionEnabled = false;
      testingActive = false;
      // Start logging while in idle state
      startLogging();
      response = "Logging idle state for 10 seconds";
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
  pinMode(analogPin, INPUT);
  analogReadResolution(12); // ESP32 has 12-bit ADC
  // Calibrate sensor
  Serial.println("Calibrating current sensor...");
  uint16_t midPoint = sensor.autoMidPoint();
  Serial.printf("Midpoint value: %d\n", midPoint);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
  // Initialize all servos
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i]);
    servos[i].write(centerPosition);
  }
  delay(1000);  // Allow servos to reach center position

  // Setup WiFi Access Point
  WiFi.softAP("ESP32_Snake", "12345678");
  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Setup server handlers
  server.on("/", HTTP_GET, handleRoot);
  server.on("/", HTTP_POST, handleSetParameters);
  server.on("/current", HTTP_GET, handleCurrentData);  
  server.on("/download", HTTP_GET, handleDownload);
  server.on("/clearlog", HTTP_GET, handleClearLog);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  // Handle servo operations
  if (testingActive) {
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
  
  // Handle logging at fixed intervals
  if (loggingEnabled) {
    // Check if logging duration has expired
    if (millis() - loggingStartTime >= LOGGING_DURATION) {
      loggingEnabled = false;
      Serial.println("Logging stopped after 10 seconds");
    } 
    // Log at fixed intervals
    else if (millis() - lastLogTime >= LOGGING_INTERVAL) {
      // Take a current reading
      float current = 0.0;
      for(int i = 0; i < 5; i++) { // Take fewer samples for speed
        current += abs(sensor.mA_DC());
      }
      current = (current / 5.0) / 1000.0;  // Convert to Amps
      
      logCurrentData(current);
      lastLogTime = millis();
    }
  }
}