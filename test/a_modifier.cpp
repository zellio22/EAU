#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>

const char* ssid = "ESP32-Terminal";
const char* password = "12345678";

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>ESP32 Terminal</title>
  <style>
    body {
      background-color: #121212;
      color: #00ff90;
      font-family: 'Courier New', monospace;
      margin: 0;
      padding: 0;
      display: flex;
      flex-direction: column;
      height: 100vh;
    }

    h2 {
      text-align: center;
      padding: 1rem;
      color: #00ff90;
      border-bottom: 1px solid #00ff90;
    }

    .terminal-box {
      flex: 1;
      background-color: #1e1e1e;
      border: 1px solid #00ff90;
      margin: 1rem;
      padding: 1rem;
      border-radius: 6px;
      display: flex;
      flex-direction: column;
    }

    #terminal {
      flex: 1;
      background-color: #000000;
      color: #00ff90;
      padding: 1rem;
      resize: none;
      border: none;
      font-size: 1rem;
      line-height: 1.4;
      overflow-y: auto;
    }

    .input-section {
      display: flex;
      padding: 1rem;
      background-color: #1e1e1e;
      gap: 0.5rem;
      border-top: 1px solid #00ff90;
    }

    input[type="text"] {
      flex: 1;
      padding: 0.5rem;
      font-size: 1rem;
      background-color: #000;
      color: #00ff90;
      border: 1px solid #00ff90;
      border-radius: 4px;
    }

    button {
      padding: 0.5rem 1rem;
      font-size: 1rem;
      background-color: #00ff90;
      color: #000;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      transition: background 0.2s;
    }

    button:hover {
      background-color: #00c170;
    }

    .button-row {
      display: flex;
      justify-content: space-around;
      padding: 1rem;
      background-color: #1e1e1e;
      border-top: 1px solid #00ff90;
    }

    .button-row button {
      flex: 1;
      margin: 0 0.5rem;
    }
  </style>
</head>
<body>
  <h2>ESP32 Terminal WebSocket</h2>

  <!-- Encadré pour le terminal -->
  <div class="terminal-box">
    <textarea id="terminal" readonly></textarea>
  </div>

  <!-- Barre de commande -->
  <div class="input-section">
    <input type="text" id="input" placeholder="Enter command">
    <button onclick="sendMessage()">Send</button>
  </div>

  <!-- Boutons de commande -->
  <div class="button-row">
    <button onclick="insertCommand('Clear_event_led')">Clear_event_led</button>
    <button onclick="insertCommand('Aq FT 4000')">Aq FT 4000</button>
    <button onclick="insertCommand('Alarm off')">Alarm off</button>
    <button onclick="insertCommand('je c pas')">je c pas</button>
  </div>

  <script>
    let socket;
    try {
      socket = new WebSocket("ws://" + location.hostname + ":81/");
    } catch (e) {
      console.warn("WebSocket not connected (ESP32 non disponible).");
    }

    const term = document.getElementById("terminal");
    const inputField = document.getElementById("input");

    if (socket) {
      socket.onmessage = function(event) {
        term.value += event.data;
        term.scrollTop = term.scrollHeight;
      };
    }

    function sendMessage() {
      const msg = inputField.value.trim();
      if (msg === "") return;

      term.value += "> " + msg + "\n";

      if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send(msg + "\n");
      } else {
        console.log("Simulated send:", msg);
      }

      term.scrollTop = term.scrollHeight;
      inputField.value = "";
    }

    function insertCommand(cmd) {
      inputField.value = cmd;
      inputField.focus();
    }
  </script>
</body>
</html>

)rawliteral";

IPAddress local_ip(192, 168, 1, 1);        // IP de l'ESP32
IPAddress gateway(192, 168, 1, 1);         // La passerelle peut être la même que l'IP
IPAddress subnet(255, 255, 255, 0);        // Masque de sous-réseau

void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_TEXT:
      Serial.write(payload, length); // envoyer au port série
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);/// Normalement ok 
  Serial.println("AP IP address: " + WiFi.softAPIP().toString());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", htmlPage);
  });

  server.begin();
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  Serial.println("Web server + WebSocket started.");
}

static String serialBuffer = "";

void loop() {
  webSocket.loop();

  while (Serial.available()) {
    char c = Serial.read();
    serialBuffer += c;

    if (c == '\n') { // on envoie ligne par ligne
      webSocket.broadcastTXT(serialBuffer);
      serialBuffer = "";
    }
  }
}
/*
char c = Serial.read();
webSocket.broadcastTXT(String(c));/// pour envoyer part chart
*/ 

/* pour envoyer par ligne 
void loop() {
  webSocket.loop();

  while (Serial.available()) {
    char c = Serial.read();
    serialBuffer += c;

    if (c == '\n') { // on envoie ligne par ligne
      webSocket.broadcastTXT(serialBuffer);
      serialBuffer = "";
    }
  }
}
*/