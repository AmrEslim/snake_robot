#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <math.h>

// Network credentials
WebServer server(80);

#define NUM_SERVOS 10  // Updated to 10 servos

Servo servos[NUM_SERVOS];
int servoPins[NUM_SERVOS] = {2, 4, 16, 17, 5, 18, 19, 21, 22, 23};

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
</
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

void testServos() {
  for (int i = 0; i < NUM_SERVOS; i++) {
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

  for (int i = 0; i < NUM_SERVOS; i++) {
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
