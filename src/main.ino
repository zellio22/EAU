#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>
#include <HardwareSerial.h>

const char* ssid = "EAU-Terminal";
const char* password = "12345678";

AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
HardwareSerial MySerial(1); // UART1
String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>EAU Terminal</title>
  <style>
    @media (orientation: portrait) {
      .terminal-box {
        width: 57rem;
        height: 10px;
      }

      #terminal {
        font-size: 2rem;
      }

      .button-row {
        flex-direction: row;
        gap: 0.5rem;
      }

      .button-row button {
        display: flex;
        text-align: center;
      }
    }

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
      margin: 0.25rem 0;
      padding: 0.25rem 0;
      color: #00ff90;
      font-size: 1rem;
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
      height: 45px;
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
      height: 45px;
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

    #dashboard {
      display: none;
      padding: 1rem;
      background-color: #1e1e1e;
      border-top: 1px solid #00ff90;
    }

    progress {
      width: 100%;
      height: 20px;
      appearance: none;
    }

    progress::-webkit-progress-bar {
      background-color: #000;
      border: 1px solid #00ff90;
      border-radius: 4px;
    }

    progress::-webkit-progress-value {
      background-color: #00ff90;
    }

    #dashboard label {
      margin-bottom: 0.5rem;
      font-size: 0.9rem;
    }

    /* Petit bouton en haut à gauche */
    #toggleBtn {
      position: absolute;
      top: 10px;
      left: 10px;
      padding: 5px 10px;
      font-size: 0.8rem;
      height: auto;
      z-index: 10;
    }
    .progress-bar {
  position: relative;
  width: 100%;
  height: 20px;
  background-color: #000;
  border: 1px solid #00ff90;
  border-radius: 4px;
}

.cursor {
  position: absolute;
  top: -3px;
  width: 10px;
  height: 26px;
  background-color: #00ff90;
  border-radius: 3px;
  transition: left 0.1s ease-out;
}

  </style>
</head>
<body>
  <!-- Petit bouton pour dashboard -->
  <button onclick="toggleDashboard()" id="toggleBtn">Afficher le Dashboard</button>

  <h2>Terminal EAU</h2>

  <!-- Terminal -->
  <div class="terminal-box" id="terminalSection">
    <textarea id="terminal" readonly></textarea>
  </div>

  <!-- Entrée de commande -->
  <div class="input-section" id="inputSection">
    <input type="text" id="input" placeholder="Enter command">
    <button onclick="sendMessage()">Send</button>
  </div>

  <!-- Boutons de commande -->
  <div class="button-row" id="commandButtons">
    <button onclick="insertCommand('Clear_event_led')">Clear_event_led</button>
    <button onclick="insertCommand('Aq FT 4000')">Aq FT 4000</button>
    <button onclick="insertCommand('Alarm off')">Alarm off</button>
    <button onclick="insertCommand('je c pas')">je c pas</button>
    <button onclick="insertCommand('le der')">5em</button>
  </div>

  <!-- Dashboard -->
  <div id="dashboard">
    <div style="display: flex; flex-direction: column; gap: 1.5rem;">
      <label>Slider :
        <input type="range" id="rangeSlider" min="0" max="100" value="50" oninput="updateProgress(this.value)">
      </label>
  
      <div class="progress-bar" id="bar1"><div class="cursor" id="cursor1"></div></div>
      <div class="progress-bar" id="bar2"><div class="cursor" id="cursor2"></div></div>
      <div class="progress-bar" id="bar3"><div class="cursor" id="cursor3"></div></div>
      <div class="progress-bar" id="bar4"><div class="cursor" id="cursor4"></div></div>
    </div>
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

    function toggleDashboard() {
      const dashboard = document.getElementById("dashboard");
      const terminal = document.getElementById("terminalSection");
      const input = document.getElementById("inputSection");
      const buttons = document.getElementById("commandButtons");
      const toggleBtn = document.getElementById("toggleBtn");

      const isDashboardVisible = dashboard.style.display === "block";

      dashboard.style.display = isDashboardVisible ? "none" : "block";
      terminal.style.display = isDashboardVisible ? "flex" : "none";
      input.style.display = isDashboardVisible ? "flex" : "none";
      buttons.style.display = isDashboardVisible ? "flex" : "none";

      toggleBtn.textContent = isDashboardVisible ? "Afficher Electro" : "Affiher terminal";
    }

    function updateProgress(val) {
      const bars = [1, 2, 3, 4];
      bars.forEach(i => {
      const cursor = document.getElementById(`cursor${i}`);
      const bar = document.getElementById(`bar${i}`);
      const width = bar.clientWidth;
      const position = (val / 100) * width;
      cursor.style.left = `${position - 5}px`; // -5px pour centrer le curseur
  });
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
      MySerial.write(payload, length); // envoyer au port série
      Serial.write(payload,length);//pour le port USB
      break;
    default:
      break;
  }
}

void setup() {


  Serial.begin(115200);
  MySerial.begin(9600, SERIAL_8N1, 18, 17); // RX=39, TX=40sendMessage()
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

  // def E/S
  pinMode(15,OUTPUT);
  pinMode(39,OUTPUT);
  pinMode(37,OUTPUT);
  pinMode(35,OUTPUT);

  //teste

  digitalWrite(15,true);
  digitalWrite(1,true);
  digitalWrite(2,false);
  digitalWrite(4,true);

}

static String serialBuffer = "";
static String serialusbBuffer = "";

unsigned long lastloop_led = millis();
unsigned long lastloop_read = millis();

int color=0;

void loop() {
  webSocket.loop();


  while (MySerial.available()) {
    char c = MySerial.read();
    serialBuffer += c;

    if (c == '\n') { // on envoie ligne par ligne
      webSocket.broadcastTXT("Serial,"+serialBuffer);
      serialBuffer = "";
    }
  }

  while (Serial.available()) {
    char c = Serial.read();
    serialusbBuffer += c;

    if (c == '\n') { // on envoie ligne par ligne
      webSocket.broadcastTXT("Serial,"+serialusbBuffer);
      serialusbBuffer = "";
    }
  }

  if (millis()>=lastloop_led + 1000){
    color=color+1;
    lastloop_led=millis();
    switch (color){
      case 1:
      digitalWrite(39,true);//green
      digitalWrite(37,false);
      digitalWrite(35,true);

      //Serial.println("case 1");
      break;

      case 2:
      digitalWrite(39,false);//red
      digitalWrite(37,true);
      digitalWrite(35,true);
      //Serial.println("case 2");
      break;

      case 3:
      digitalWrite(39,false);//orange
      digitalWrite(37,false);
      digitalWrite(35,true);
      //Serial.println("case 3");
      break;

      case 4:
      digitalWrite(39,false);
      digitalWrite(37,false);
      digitalWrite(35,false);//Off ok
      //Serial.println("case 4");
      color=0;
      break;
    }

    
  }

  if(millis()>=lastloop_read + 200){
    lastloop_read=millis();
    Serial.print("ana1= ");
    Serial.println(analogRead(11));
    Serial.print("ana2= ");
    Serial.println(analogRead(13));
    Serial.print("ana3= ");
    Serial.println(analogRead(2));
    Serial.print("ana4= ");
    Serial.println(analogRead(3));
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