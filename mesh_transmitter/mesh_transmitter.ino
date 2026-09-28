/*
  MESH TRANSMITTER - Heartbeat Demo (ESP32 + painlessMesh)

  Broadcasts a heartbeat every HEARTBEAT_INTERVAL_MS and blinks the
  onboard LED on each send. Also sends immediately when a connection
  forms or the mesh topology changes, so receivers resume quickly
  after the transmitter is power-cycled.

  Libraries: painlessMesh, ArduinoJson, TaskScheduler, AsyncTCP
  Each set needs its own MESH_PREFIX (heartbeatA / heartbeatB).
*/

#include "painlessMesh.h"

// ---------- Config ----------
#define MESH_PREFIX    "heartbeatA"
#define MESH_PASSWORD  "meshdemo1234"
#define MESH_PORT      5555

#define LED_PIN        2
#define LED_ON_MS      200
#define HEARTBEAT_INTERVAL_MS 1000
#define POST_CONNECT_RESEND_MS 500

#define REDUCE_TX_POWER false

// ---------- Globals ----------
Scheduler userScheduler;
painlessMesh mesh;

uint32_t seqNum = 0;

void sendHeartbeat();
void ledOff();

Task taskHeartbeat(TASK_MILLISECOND * HEARTBEAT_INTERVAL_MS, TASK_FOREVER, &sendHeartbeat);
Task taskLedOff(TASK_MILLISECOND * 1, TASK_ONCE, &ledOff);

void ledOff() {
  digitalWrite(LED_PIN, LOW);
}

void sendHeartbeat() {
  String msg = "HB|" + String(mesh.getNodeId()) + "|" + String(seqNum++);
  mesh.sendBroadcast(msg);

  digitalWrite(LED_PIN, HIGH);
  taskLedOff.restartDelayed(LED_ON_MS);

  Serial.printf("Sent heartbeat: %s (nodes in mesh: %d)\n",
                msg.c_str(), (int)mesh.getNodeList().size());
}

// Send right now, then again shortly after, then resume normal cadence
void kickHeartbeat() {
  sendHeartbeat();
  taskHeartbeat.restartDelayed(POST_CONNECT_RESEND_MS);
}

// ---------- Mesh callbacks ----------
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New connection: %u\n", nodeId);
  kickHeartbeat();
}

void changedConnectionCallback() {
  Serial.printf("Mesh topology changed. Nodes: %d\n",
                (int)mesh.getNodeList().size());
  kickHeartbeat();
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);

#if REDUCE_TX_POWER
  WiFi.setTxPower(WIFI_POWER_2dBm);
#endif

  userScheduler.addTask(taskHeartbeat);
  userScheduler.addTask(taskLedOff);
  taskHeartbeat.enable();
}

void loop() {
  mesh.update();
}