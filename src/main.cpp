#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// 15 seconds timeout for connection to AP
#define CONNECT_TIMEOUT 10 * 1000
#define WIFI_RECONNECT_INTERVAL 30 * 1000
#define LEFT_WINDOWS 12
#define RIGHT_WINDOWS 14
#define REPORT_TOPIC "esp8266_report_windows"


// Message format
#define MSG_WINDOWS_STATE "{\"left\":{\"state\":\"%s\"},\"right\":{\"state\":\"%s\"}}"
#define WINDOWS_STATE_OPENED "OPENED"
#define WINDOWS_STATE_CLOSED "CLOSED"

// Use INPUT_PULLUP, so initialize state is HIGH
bool windowsLeftState = HIGH;
bool windowsRightState = HIGH;

int lastWiFiReconnect = 0;
// Your SSID
const char* ssid = "Something";
// Your PSK
const char* password = "Something";

// IOT server
const char* server = "iot.eclipse.org";

WiFiClient wifiClient;
PubSubClient client(wifiClient);
void updateWindowsStates() {
  windowsLeftState = digitalRead(LEFT_WINDOWS);
  windowsRightState = digitalRead(RIGHT_WINDOWS);
}

void postWindowsStateMessage() {
  char * msg = (char*) malloc(50 * sizeof(char));
  sprintf(msg, MSG_WINDOWS_STATE,
    windowsLeftState ? WINDOWS_STATE_OPENED : WINDOWS_STATE_CLOSED,
    windowsRightState ? WINDOWS_STATE_OPENED : WINDOWS_STATE_CLOSED);
  if (client.connected()) {
    client.publish(REPORT_TOPIC,msg);
  }
  delete msg;
}

void connectToWiFi() {
  lastWiFiReconnect = millis();
  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  int start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if ((millis() - start) > CONNECT_TIMEOUT) {
      break;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    client.setServer(server, 1883);
  } else {
    Serial.println("Failed");
  }
}

void connectToBroker() {
  Serial.print("Attempting MQTT connection...");
  String clientId = "ESP8266Client-";
  clientId += String(random(0xffff), HEX);
  if (client.connect(clientId.c_str())) {
    Serial.println("connected");
    postWindowsStateMessage();
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println("");
  }
}



void setup() {
  Serial.begin(115200);
  Serial.println("Begin...");
  pinMode(LEFT_WINDOWS, INPUT_PULLUP);
  pinMode(RIGHT_WINDOWS, INPUT_PULLUP);
  windowsLeftState = digitalRead(LEFT_WINDOWS);
  windowsRightState = digitalRead(RIGHT_WINDOWS);
  connectToWiFi();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED && ((millis() - lastWiFiReconnect) > WIFI_RECONNECT_INTERVAL)) {
    connectToWiFi();
  }
  if (WiFi.status() == WL_CONNECTED && !client.connected()) {
    connectToBroker();
  }

  if (client.connected()) {
    client.loop();
  }
  bool changed = false;
  if (windowsLeftState != digitalRead(LEFT_WINDOWS) || windowsRightState != digitalRead(RIGHT_WINDOWS)) {
    updateWindowsStates();
    postWindowsStateMessage();
  }
}
