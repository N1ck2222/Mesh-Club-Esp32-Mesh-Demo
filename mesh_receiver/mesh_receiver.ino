/*
  MESH RECEIVER - Heartbeat Demo (ESP32 + painlessMesh)

  Flash this SAME sketch onto both receiver boards in a set.

    - Heartbeat received recently -> onboard LED flashes FAST
    - No heartbeat for a while    -> LED stays OFF

  painlessMesh relays broadcasts automatically, so no relay code needed.

  Libraries: painlessMesh, ArduinoJson, TaskScheduler, AsyncTCP
  MESH_PREFIX / MESH_PASSWORD / MESH_PORT must match this set's transmitter.
*/

#include "painlessMesh.h"

// ---------- Config ----------
#define MESH_PREFIX    "heartbeatA"
#define MESH_PASSWORD  "meshdemo1234"
#define MESH_PORT      5555

#define LED_PIN        2
#define FAST_BLINK_MS  100
#define HEARTBEAT_TIMEOUT_MS 2000   // transmitter sends every 1s, so ~2 missed beats

#define REDUCE_TX_POWER false

// ---------- Globals ----------
Scheduler userScheduler;
painlessMesh mesh;

uint32_t lastHeartbeatMs = 0;
bool everReceived = false;
bool wasReceiving = false;
bool ledState = false;

void blinkLed();
Task taskBlink(TASK_MILLISECOND * FAST_BLINK_MS, TASK_FOREVER, &blinkLed);

bool isReceiving() {
  return everReceived && (millis() - lastHeartbeatMs) < HEARTBEAT_TIMEOUT_MS;
}

void blinkLed() {
  bool receiving = isReceiving();

  if (receiving) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  } else {
    ledState = false;
    digitalWrite(LED_PIN, LOW);
  }

  if (receiving != wasReceiving) {
    wasReceiving = receiving;
    Serial.println(receiving ? "Heartbeat: RECEIVING" : "Heartbeat: LOST");
  }
}

// ---------- Mesh callbacks ----------
void receivedCallback(uint32_t from, String &msg) {
  if (msg.startsWith("HB|")) {
    lastHeartbeatMs = millis();
    everReceived = true;
    Serial.printf("Heartbeat from node %u: %s\n", from, msg.c_str());
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New connection: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Mesh topology changed. Nodes: %d\n",
                (int)mesh.getNodeList().size());
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

#if REDUCE_TX_POWER
  WiFi.setTxPower(WIFI_POWER_2dBm);
#endif

  userScheduler.addTask(taskBlink);
  taskBlink.enable();
}

void loop() {
  mesh.update();
}