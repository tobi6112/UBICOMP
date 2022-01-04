#include <Arduino.h>

// Secrets
#include <Credentials.h>

// WiFi
#include <WiFi.h>

// MQTT
#include <AsyncMqttClient.h>

// JSON
#include <ArduinoJson.h>

// Grove NFC
#include <Wire.h>
#include <MFRC522.h>

// IR
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRCodes.h>

// LCD
#include "rgb_lcd.h"

// LED Pin
const int LED = 16;
// IR Pin
const int IR_RECV = 33;

const String NODE_IDENTIFIER = String("node-1");
const String COMMAND_TOPIC = String("node/") + NODE_IDENTIFIER + String("/set");
const String SENSOR_TOPIC = String("node/") + NODE_IDENTIFIER + String("/sensor");
const String IR_SENSOR_TOPIC = SENSOR_TOPIC + String("/ir"); 

const boolean DEBUG = true;

AsyncMqttClient mqttClient;
boolean tmp_enabled = false; // TODO: Just for testing
IRrecv irReceiver(IR_RECV);
// IRfrequency irFreq(IR_FREQ);
decode_results results;
rgb_lcd lcd;

#pragma region Type definitons

typedef struct s_ir_data {
  int code;
  int command;
} t_ir_data;

enum Command {
  DISABLE = 0,
  ENABLE = 1
};

typedef struct s_node_command {
  const char* node;
  Command command;
} t_node_command;

#pragma endregion

#pragma region Helper and Debugging Utils

void i2c_scanner()
{
  Serial.println();
  Serial.println("I2C scanner. Scanning ...");
  byte count = 0;

  Wire.begin();
  for (byte i = 1; i < 120; i++)
  {
    Wire.beginTransmission(i);

    if (Wire.endTransmission() == 0)
    {
      Serial.print("Found address: ");
      Serial.print(i, DEC);
      Serial.print(" (0x");
      Serial.print(i, HEX);
      Serial.println(")");
      count++;
      delay(1);
    }
  }
  Serial.println("Done.");
  Serial.print("Found ");
  Serial.print(count, DEC);
  Serial.println(" device(s).");
}

void connectToWifi()
{
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

#pragma endregion

#pragma region Collect Sensor Data

void setupSensors() {
  // Enable IR
  irReceiver.enableIRIn();
}

t_ir_data* readIR()
{
  if (irReceiver.decode(&results))
  {
    int code = (int)strtol(resultToHexidecimal(&results).c_str(), NULL, 0);
    int command = (&results)->command;
    t_ir_data *result;
    result = (t_ir_data*) malloc(sizeof(t_ir_data));
    result->code = code;
    result->command = command;

    if(DEBUG) {
      Serial.print("Code (Hex): ");
      Serial.println(code);
      Serial.print("Command: ");
      Serial.println(command);
      Serial.print(resultToHumanReadableBasic(&results));
      Serial.println(resultToSourceCode(&results));
    }

    irReceiver.resume();

    return result;
  }
  return nullptr;
}

void readAndPublishSensors() {
  t_ir_data* ir_data = readIR();
  if(ir_data) {
    DynamicJsonDocument doc(512);
    doc["code"] = ir_data->code; 
    doc["command"] = ir_data->command;
    String json;
    serializeJson(doc, json);
    mqttClient.publish(SENSOR_TOPIC.c_str(), 0, false, json.c_str());
  }
}

#pragma endregion

#pragma region Command Event Handlers

void setupPresentation() {
  // LCD
  lcd.begin(16, 2);
}

void onEnableNode() {
  if(DEBUG) {
    Serial.println("Node enabled");
  }

  digitalWrite(LED, HIGH);

  lcd.setRGB(0, 255, 0);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("ENABLED");
}

void onDisableNode() {
  if(DEBUG) {
    Serial.println("Node disabled");
  }

  digitalWrite(LED, LOW);

  lcd.setRGB(255, 0, 0);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DISABLED");
}

void onCommand(t_node_command* command) {
  String node = String(command->node);
  Command cmd = command->command;

  if(node.equals(NODE_IDENTIFIER)) {
    switch (cmd)
    {
      case DISABLE:
        onDisableNode();
        break;
      
      case ENABLE:
        onEnableNode();
        break;
      
      default:
        Serial.println("Unsupported command");
        break;
    }
  } else {
    Serial.print(node);
    Serial.print(" does not match node identifier ");
    Serial.println(cmd);
  }
}
#pragma endregion

#pragma region Other Event Handler

void onMqttConnect(bool sessionPresent)
{
  if(DEBUG) {
    Serial.println("Connected to MQTT.");
    Serial.print("Session present: ");
    Serial.println(sessionPresent);

    uint16_t packetIdSub = mqttClient.subscribe(COMMAND_TOPIC.c_str(), 2);
    Serial.print("Subscribing at QoS 2, packetId: ");
    Serial.println(packetIdSub);
  }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  if(DEBUG) {
    Serial.println("Disconnected from MQTT.");
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos)
{
  if(DEBUG) {
    Serial.println("Subscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
    Serial.print("  qos: ");
    Serial.println(qos);
  }
}

void onMqttUnsubscribe(uint16_t packetId)
{
  if(DEBUG) {
    Serial.println("Unsubscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
  }
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{
  if(DEBUG) {
    Serial.println("Publish received.");
    Serial.print("  topic: ");
    Serial.println(topic);
    Serial.print("  qos: ");
    Serial.println(properties.qos);
    Serial.print("  dup: ");
    Serial.println(properties.dup);
    Serial.print("  retain: ");
    Serial.println(properties.retain);
    Serial.print("  len: ");
    Serial.println(len);
    Serial.print("  index: ");
    Serial.println(index);
    Serial.print("  total: ");
    Serial.println(total);
    Serial.print("  payload: ");
    Serial.println(payload);
  }

  if(String(topic).equals(String(COMMAND_TOPIC))) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.println("Failed to read payload");
      Serial.println(error.f_str());
    } else {
      t_node_command *command;
      command = (t_node_command*) malloc(sizeof(t_node_command));
      
      const char* node = doc["node"];
      int cmd = doc["command"];

      command->node = node;
      command->command = (Command) cmd;

      if(DEBUG) {
        Serial.println("Parsed Command");
        Serial.print("Node: ");
        Serial.println(command->node);
        Serial.print("Command: ");
        Serial.println(command->command);
      }

      onCommand(command);
    }
  } else {
    if(DEBUG) {
      Serial.println("No Matching topic");
    }
  }
}

void onMqttPublish(uint16_t packetId)
{
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onWiFiEvent(WiFiEvent_t event)
{
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch (event)
  {
  case SYSTEM_EVENT_STA_GOT_IP:
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("MAC address: ");
    Serial.println(WiFi.macAddress());
    connectToMqtt();
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    Serial.println("WiFi lost connection");
    break;
  default:
    break;
  }
}

#pragma endregion

#pragma region Arduino Lifecycle

void setup()
{
  Serial.begin(115200);

  while (!Serial)
  {
  }

  if(DEBUG) {
    i2c_scanner();
  }

  pinMode(LED, OUTPUT);

  // Register Events
  WiFi.onEvent(onWiFiEvent);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  // Connect to WiFi - MQTT Connection will be established aswell
  connectToWifi();

  setupSensors();
  setupPresentation();
}

void loop()
{
  readAndPublishSensors();
}

#pragma endregion