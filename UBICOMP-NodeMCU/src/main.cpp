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
#include <PN532/PN532_I2C/PN532_I2C.h>
#include <NfcAdapter.h>

// IR
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRCodes.h>

// LCD
#include "rgb_lcd.h"

/***** Constants ******/
#pragma region
/**************************************************************************/
// This is the node identifier (or qualifier). It should be unique
const String NODE_IDENTIFIER = String("node-1");
// The PIN used for LED presentation
const int LED = 16;
// IR Pin
const int IR_RECV = 33;
// Sound Sensor Pin
const int SOUND = 34;
// NFC Timeout
const int NFC_TIMEOUT = 0x14;

/**************************************************************************/
#pragma endregion


/***** Feature / Build Flags ******/
#pragma region
/**************************************************************************/
// These are feature flags to enable node-specific features.
// TODO: We probably would rather inline them using build profiles
//       to even exclude libraries. However, this would cruttle the codebase as we're using
//       a single-file sketch and would require more modularity to be useful
#ifdef S_NFC
const boolean S_NFC_ENABLED = true;
#else 
const boolean S_NFC_ENABLED = false;
#endif

#ifdef S_IR
const boolean S_IR_ENABLED = true;
#else 
const boolean S_IR_ENABLED = false;
#endif

#ifdef S_SOUND
const boolean S_SOUND_ENABLED = true;
#else 
const boolean S_SOUND_ENABLED = false;
#endif

#ifdef P_LCD
const boolean P_LCD_ENABLED = true;
#else 
const boolean P_LCD_ENABLED = false;
#endif

#ifdef P_LED
const boolean P_LED_ENABLED = true;
#else 
const boolean P_LED_ENABLED = false;
#endif

#ifdef DEBUG_MODE
const boolean DEBUG = true;
#else 
const boolean DEBUG = false;
#endif

/**************************************************************************/
#pragma endregion

/***** MQTT Topics ******/
#pragma region
/**************************************************************************/
const String COMMAND_TOPIC = String("node/") + NODE_IDENTIFIER + String("/set");
const String STATE_TOPIC = String("node/") + NODE_IDENTIFIER + String("/state");
const String SENSOR_TOPIC = String("node/") + NODE_IDENTIFIER + String("/sensor");
const String IR_SENSOR_TOPIC = SENSOR_TOPIC + String("/ir"); 
const String SOUND_SENSOR_TOPIC = SENSOR_TOPIC + String("/sound");
const String NFC_SENSOR_TOPIC = SENSOR_TOPIC + String("/nfc");
/**************************************************************************/
#pragma endregion

/***** Adapters ******/
#pragma region
/**************************************************************************/
AsyncMqttClient mqttClient;
IRrecv irReceiver(IR_RECV);
decode_results results;
rgb_lcd lcd;
PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
/**************************************************************************/
#pragma endregion

/***** Statevars / Helpers ******/
#pragma region
/**************************************************************************/
boolean enabled = false;
boolean nfcMutex = false;
/**************************************************************************/
#pragma endregion


/***** Type Definitions ******/
#pragma region
/**************************************************************************/

// Data that we get from the IR Sensor
typedef struct s_ir_data {
  uint32_t code;
  uint16_t command;
} t_ir_data;

// Data that we get from the Sound Sensor
typedef struct s_sound_data {
  uint16_t value;
} t_sound_data;

// Data that we get from the NFC Sensor
typedef struct s_nfc_data {
  const char* uid;
  const char* type;
} t_nfc_data;

// Possible Node States
enum State {
  OFF = 0,
  ON = 1
};

// Command types that can be performed on this node
enum Command {
  DISABLE = 0,
  ENABLE = 1,
  TOGGLE = 2
};

// How a command will be received
typedef struct s_node_command {
  const char* node;
  Command command;
} t_node_command;
/**************************************************************************/
#pragma endregion


/***** Debugger / Utils ******/
#pragma region 
/**************************************************************************/

/**
 * @brief Scans for I²C devices and prints their Addresses
 */
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

/**
 * @brief connect to wifi with the provided credentials
 * 
 */
void connectToWifi()
{
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

/**
 * @brief connect to MQTT
 * 
 */
void connectToMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}
/**************************************************************************/

#pragma endregion

/***** Sensors ******/
#pragma region Collect Sensor Data
/**************************************************************************/

/**
 * @brief Helper method to setup the sensors
 * 
 */
void setupSensors() {
  // Print a warning when NFC and IR are enabled.
  // We experienced interferences and inconsistent behaviour when both are 
  // enabled at the same time - probably because the NFC reader blocks the 
  // whole execution. Even if a timout is specified the IR sensor might not
  // work as resilient as expected
  if(S_IR_ENABLED && S_NFC_ENABLED) {
    Serial.println("NFC and IR enabled, NFC blocks the execution - so IR might not work properly");
  }

  if(S_IR_ENABLED) {
    if(DEBUG) {
      Serial.println("Enabling IR...");
    }
    irReceiver.enableIRIn();
  }

  if(S_NFC_ENABLED) {
    if(DEBUG) {
      Serial.println("Enabling NFC...");
    }
    nfc.begin();
  }
}

/**
 * @brief Read the expected data from NFC tag
 * 
 * @return t_nfc_data* data read from NFC tag as defined in the typedef.
 *                     nullptr if no tag was present.
 */
t_nfc_data* readNFC() {
  // We're using a simple mutex to prevent the loop from reading the same tag
  // over and over again if it is held against the antenna. 
  if (nfc.tagPresent(NFC_TIMEOUT) && !nfcMutex) {
    nfcMutex = true;
    NfcTag tag = nfc.read();
    String type = tag.getTagType();
    String uid = tag.getUidString();
    
    // By default the UID won't be separated by colons - which I found fancier
    uid.replace(" ", ":");
    
    if(DEBUG) {
      Serial.println("Read NFC");
      Serial.print("Type: ");
      Serial.println(type);
      Serial.print("UID: ");
      Serial.println(uid);
    }

    // Build Data and return
    t_nfc_data *result;
    result = (t_nfc_data*) malloc(sizeof(t_nfc_data));
    result->type = type.c_str();
    result->uid = uid.c_str();

    return result;
  }
  
  // We're freeing the mutex if no tag is present
  if(!nfc.tagPresent(NFC_TIMEOUT)) {
    nfcMutex = false;
  }
  return nullptr;
}

/**
 * @brief Read the expected data from IR sensor
 * 
 * @return t_ir_data* data read from IR signal tag as defined in the typedef.
 *                     nullptr if no signal was present.
 */
t_ir_data* readIR()
{
  if (irReceiver.decode(&results))
  {
    uint32_t code = (uint32_t)strtol(resultToHexidecimal(&results).c_str(), NULL, 0);
    uint16_t command = (&results)->command;
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

/**
 * @brief Read Data from the sound sensor
 * 
 * @return t_sound_data* data read from sound sensor tag as defined in the typedef.
 */
t_sound_data* readSound() {
  uint16_t measurement = analogRead(SOUND);
  t_sound_data *result = (t_sound_data*) malloc(sizeof(t_sound_data));
  result->value = measurement;
  return result;
}

/**
 * @brief Reads Data from all enabled sensors. If data is present they will be streamlined
 *        into their defined sensor topic.
 * 
 */
void readAndPublishSensors() {
  if(S_IR_ENABLED) {
    t_ir_data* ir_data = readIR();
    if(ir_data) {
      // Parse data to JSON
      DynamicJsonDocument doc(512);
      doc["code"] = ir_data->code; 
      doc["command"] = ir_data->command;
      String json = String("");
      serializeJson(doc, json);

      // Publish under IR Sensor Topic
      mqttClient.publish(IR_SENSOR_TOPIC.c_str(), 0, false, json.c_str());
    }
  }
  if(S_SOUND_ENABLED) {
    t_sound_data* sound_data = readSound();
    if(sound_data) {
      // Parse data to JSON
      DynamicJsonDocument doc(512);
      doc["value"] = sound_data->value; 
      String json = String("");
      serializeJson(doc, json);

      // Publish under IR Sensor Topic
      mqttClient.publish(SOUND_SENSOR_TOPIC.c_str(), 0, false, json.c_str());
    }
  }
  if(S_NFC_ENABLED) {
    t_nfc_data* nfc_data = readNFC();
    if(nfc_data) {
      // Parse data to JSON
      DynamicJsonDocument doc(512);
      doc["uid"] = nfc_data->uid; 
      doc["type"] = nfc_data->type; 
      String json = String("");
      serializeJson(doc, json);
      
      // Publish under IR Sensor Topic
      mqttClient.publish(NFC_SENSOR_TOPIC.c_str(), 0, false, json.c_str());
    }
  }
}

/**************************************************************************/
#pragma endregion


/***** Command Event Handlers ******/
#pragma region
/**************************************************************************/

/**
 * @brief Publish a node state to mqtt
 * 
 * @param state state to be published
 */
void publishState(State state) {
  // Build JSON
  DynamicJsonDocument doc(128);
  doc["state"] = state;
  doc["node"] = NODE_IDENTIFIER.c_str(); 
  String json = String("");
  serializeJson(doc, json);

  // Publish into state Topic. Retain last message
  mqttClient.publish(STATE_TOPIC.c_str(), 0, true, json.c_str());
}

/**
 * @brief 
 * 
 */
void setupPresentation() {
  if(P_LCD_ENABLED) {
    // LCD
    lcd.begin(16, 2);
    lcd.display();
  }
}

/**
 * @brief Enables a node - therfore triggers the presentation,
 *        publishes a new state and in the future possibly activate
 *        other sensors like microphone.
 * 
 */
void onEnableNode() {
  if(DEBUG) {
    Serial.println("Node enabled");
  }

  enabled = true;

  if(P_LED_ENABLED) { 
    digitalWrite(LED, HIGH);
  }

  if(P_LCD_ENABLED) {
    lcd.setRGB(0, 255, 0);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ENABLED");
  }

  publishState(ON);
}

/**
 * @brief Disables a node - therfore disables the presentation,
 *        publishes a new state and in the future possibly deactivate
 *        other sensors like microphone.
 * 
 */
void onDisableNode() {
  if(DEBUG) {
    Serial.println("Node disabled");
  }

  enabled = false;
  
  if(P_LED_ENABLED) {
    digitalWrite(LED, LOW);
  }

  if(P_LCD_ENABLED) {
    lcd.setRGB(255, 0, 0);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DISABLED");
  }

  publishState(OFF);
}

/**
 * @brief If the node is toggeled
 * 
 */
void onToggleNode() {
  if(DEBUG) {
    Serial.println("Node toggled");
  }

  // Just do the opposite of the current state lol
  if(enabled) {
    onDisableNode();
  } else {
    onEnableNode();
  }
}

/**
 * @brief The command handler which handles received commmands
 * 
 * @param command command received
 */
void onCommand(t_node_command* command) {
  // What is the affected node?
  String node = String(command->node);
  // Which command?
  Command cmd = command->command;

  // Am I affected?
  if(node.equals(NODE_IDENTIFIER)) {
    // Well okay, what should I do?
    switch (cmd)
    {
      case DISABLE:
        onDisableNode();
        break;
      
      case ENABLE:
        onEnableNode();
        break;

      case TOGGLE:
        onToggleNode();
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
/**************************************************************************/
#pragma endregion


/***** Infrastructure Event Handlers ******/
#pragma region
/**************************************************************************/

// Callback for the MQTT Connection Event
void onMqttConnect(bool sessionPresent)
{
  if(DEBUG) {
    Serial.println("Connected to MQTT.");
    Serial.print("Session present: ");
    Serial.println(sessionPresent);

    // We subscribe in the callback to re-subscribe on re-connection
    uint16_t packetIdSub = mqttClient.subscribe(COMMAND_TOPIC.c_str(), 2);
    Serial.print("Subscribing at QoS 2, packetId: ");
    Serial.println(packetIdSub);
  }
}

// Callback for disconnection - lame
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  if(DEBUG) {
    Serial.println("Disconnected from MQTT.");
  }
}

// Callback for subscription - lame
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

// Callback for unsubscrition - lame
void onMqttUnsubscribe(uint16_t packetId)
{
  if(DEBUG) {
    Serial.println("Unsubscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
  }
}

// Callback for a received message
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

  // If the message was received on the command topic
  if(String(topic).equals(String(COMMAND_TOPIC))) {
    // Parse JSON Content
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.println("Failed to read payload");
      Serial.println(error.f_str());
    } else {
      
      // Parse into the command type
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

      // Call the command handler with the received command
      onCommand(command);
    }
  } else {
    if(DEBUG) {
      Serial.println("No Matching topic");
    }
  }
}

// Callback for publish event - lame
void onMqttPublish(uint16_t packetId)
{
  if(DEBUG) {
    Serial.println("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
  }
}

// Callback for wifi events
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

      // We're connecting to the MQTT Broker right after a WiFi Connection was established
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      break;
    default:
      break;
  }
}
/**************************************************************************/
#pragma endregion

/***** Arduino Lifecycle ******/
#pragma region

/**************************************************************************/
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

  onDisableNode();
}

void loop()
{
  readAndPublishSensors();
}

/**************************************************************************/
#pragma endregion