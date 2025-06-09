#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "";
const char* password = "";


WebServer server(80);

// GPIOs
const int relay1Pin = 26;
const int relay2Pin = 27;
const int button1Pin = 14;
const int button2Pin = 12;

bool relay1State = false;
bool relay2State = false;

bool lastButton1 = HIGH;
bool lastButton2 = HIGH;
unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
const unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(115200);

  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);

  digitalWrite(relay1Pin, HIGH);
  digitalWrite(relay2Pin, HIGH);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected. IP: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/toggle", handleToggle);
  server.on("/state", handleState);
  server.begin();
}

void loop() {
  server.handleClient();
  handleButtons();
}

void handleButtons() {
  int b1 = digitalRead(button1Pin);
  int b2 = digitalRead(button2Pin);

  if (b1 != lastButton1 && b1 == LOW && millis() - lastDebounceTime1 > debounceDelay) {
    relay1State = !relay1State;
    digitalWrite(relay1Pin, relay1State ? LOW : HIGH);
    lastDebounceTime1 = millis();
  }
  lastButton1 = b1;

  if (b2 != lastButton2 && b2 == LOW && millis() - lastDebounceTime2 > debounceDelay) {
    relay2State = !relay2State;
    digitalWrite(relay2Pin, relay2State ? LOW : HIGH);
    lastDebounceTime2 = millis();
  }
  lastButton2 = b2;
}

void handleRoot() {
  server.send(200, "text/html", R"rawliteral(
    <!DOCTYPE html><html><head>
    <title>ESP32 Relay Control</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body { font-family: Arial; text-align: center; padding-top: 40px; }
      .btn {
        font-size: 20px; padding: 15px 40px; margin: 10px;
        border: none; border-radius: 5px; cursor: pointer;
      }
      .on { background-color: #4CAF50; color: white; }
      .off { background-color: #f44336; color: white; }
    </style>
    </head><body>
    <h2>ESP32 Dual Relay Control</h2>
    <button id="relay1" class="btn" onclick="toggleRelay(1)">Relay 1</button><br>
    <button id="relay2" class="btn" onclick="toggleRelay(2)">Relay 2</button>

    <script>
      function toggleRelay(r) {
        fetch("/toggle?relay=" + r).then(updateButtons);
      }

      function updateButtons() {
        fetch("/state").then(res => res.json()).then(data => {
          setButton('relay1', data.relay1);
          setButton('relay2', data.relay2);
        });
      }

      function setButton(id, state) {
        let btn = document.getElementById(id);
        if (state) {
          btn.className = "btn on";
          btn.innerText = id.replace("relay", "Relay ") + " ON";
        } else {
          btn.className = "btn off";
          btn.innerText = id.replace("relay", "Relay ") + " OFF";
        }
      }

      updateButtons();
      setInterval(updateButtons, 1000);
    </script>
    </body></html>
  )rawliteral");
}

void handleToggle() {
  String r = server.arg("relay");
  if (r == "1") {
    relay1State = !relay1State;
    digitalWrite(relay1Pin, relay1State ? LOW : HIGH);
  } else if (r == "2") {
    relay2State = !relay2State;
    digitalWrite(relay2Pin, relay2State ? LOW : HIGH);
  }
  server.send(200, "text/plain", "OK");
}

void handleState() {
  String json = "{\"relay1\":" + String(relay1State ? "true" : "false") +
                ",\"relay2\":" + String(relay2State ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}
