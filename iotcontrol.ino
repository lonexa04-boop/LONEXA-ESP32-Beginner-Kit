
#include <WiFi.h>
#include <WebServer.h>

// ESP32 GPIO connections
const int RED_LED    = 16;
const int GREEN_LED  = 4;
const int BUZZER     = 26;
const int SWITCH_PIN = 25;

// ESP32 Wi-Fi hotspot
const char* ap_ssid = "LONEXA-ESP32";
const char* ap_password = "Lonexa123";

WebServer server(80);

bool redState = false;
bool greenState = false;
bool buzzerState = false;

// Webpage
const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>LONEXA ESP32 Control</title>

<style>
body {
  font-family: Arial, sans-serif;
  background: #eef2f7;
  text-align: center;
  margin: 0;
  padding: 20px;
}
.container {
  max-width: 420px;
  margin: auto;
  background: white;
  padding: 22px;
  border-radius: 18px;
  box-shadow: 0 4px 15px #0002;
}
h2 { color: #12345a; }
button {
  width: 100%;
  padding: 16px;
  margin: 9px 0;
  font-size: 17px;
  border: none;
  border-radius: 10px;
  color: white;
  cursor: pointer;
}
.red { background: #dc3545; }
.green { background: #198754; }
.buzzer { background: #34495e; }
.status {
  background: #f1f5f9;
  padding: 15px;
  margin-top: 10px;
  border-radius: 10px;
  font-size: 18px;
}
.small { color: #667085; font-size: 13px; }
</style>
</head>

<body>
<div class="container">
  <h2>LONEXA</h2>
  <p>ESP32 Web Control Panel</p>

  <button class="red" onclick="toggle('red')">
    Red LED: <span id="red">OFF</span>
  </button>

  <button class="green" onclick="toggle('green')">
    Green LED: <span id="green">OFF</span>
  </button>

  <button class="buzzer" onclick="toggle('buzzer')">
    Buzzer: <span id="buzzer">OFF</span>
  </button>

  <div class="status">
    Switch Status: <strong id="sw">Reading...</strong>
  </div>

  <p class="small">Switch status updates automatically.</p>
  <p class="small" id="connection">Connecting...</p>
</div>

<script>
async function updateStatus() {
  try {
    const response = await fetch('/status');
    if (!response.ok) throw new Error('HTTP error');

    const data = await response.json();

    document.getElementById('red').textContent =
      data.red ? 'ON' : 'OFF';

    document.getElementById('green').textContent =
      data.green ? 'ON' : 'OFF';

    document.getElementById('buzzer').textContent =
      data.buzzer ? 'ON' : 'OFF';

    document.getElementById('sw').textContent =
      data.sw ? 'PRESSED' : 'RELEASED';

    document.getElementById('connection').textContent =
      'ESP32 connected';

  } catch (e) {
    document.getElementById('connection').textContent =
      'Connection lost. Reconnecting...';
  }
}

async function toggle(device) {
  try {
    await fetch('/toggle?device=' + device);
    await updateStatus();
  } catch (e) {
    document.getElementById('connection').textContent =
      'Command failed';
  }
}

updateStatus();
setInterval(updateStatus, 500);
</script>
</body>
</html>
)rawliteral";

// Send current switch and output states
void handleStatus() {
  bool switchPressed = (digitalRead(SWITCH_PIN) == LOW);

  String json = "{";
  json += "\"red\":" + String(redState ? "true" : "false") + ",";
  json += "\"green\":" + String(greenState ? "true" : "false") + ",";
  json += "\"buzzer\":" + String(buzzerState ? "true" : "false") + ",";
  json += "\"sw\":" + String(switchPressed ? "true" : "false");
  json += "}";

  server.send(200, "application/json", json);
}

// Handle button presses from the webpage
void handleToggle() {
  if (!server.hasArg("device")) {
    server.send(400, "text/plain", "Missing device");
    return;
  }

  String device = server.arg("device");

  if (device == "red") {
    redState = !redState;
    digitalWrite(RED_LED, redState ? HIGH : LOW);
  }
  else if (device == "green") {
    greenState = !greenState;
    digitalWrite(GREEN_LED, greenState ? HIGH : LOW);
  }
  else if (device == "buzzer") {
    buzzerState = !buzzerState;
    digitalWrite(BUZZER, buzzerState ? HIGH : LOW);
  }
  else {
    server.send(400, "text/plain", "Unknown device");
    return;
  }

  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // Assumes switch connects GPIO25 to GND when pressed
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BUZZER, LOW);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println();
  Serial.println("LONEXA ESP32 Web Control");
  Serial.print("Wi-Fi name: ");
  Serial.println(ap_ssid);
  Serial.print("Wi-Fi password: ");
  Serial.println(ap_password);
  Serial.print("Open browser at: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", webpage);
  });

  server.on("/status", HTTP_GET, handleStatus);
  server.on("/toggle", HTTP_GET, handleToggle);

  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
}
