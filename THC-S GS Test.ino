// *****************************************************************************************************************
// *****************************************************************************************************************
// MASTER
// Pirate Controls THC-S Sensor Code
// Version GS Test for CTW Soil Sensor with 0.9 OLED or 1.3 OLED
// By @n0socialskills
// Last Updated: 2025-05-10
//
// *****************************************************************************************************************
// *****************************************************************************************************************
// 
//
// This code is a modification of the code found here (https://github.com/kromadg/soil-sensor),
// https://www.beanbasement.nl/threads/diy-low-cost-tdr-moisture-content-temp-ec-substrate-sensor.18337/ and
// https://scienceinhydroponics.com/2023/01/connecting-a-low-cost-tdr-moisture-content-ec-temp-sensor-to-a-nodemcuv3.html
// Thanks to everyone involved!!!
//
// This Code is AS IS. It could be improved but it just works. Dont fuck around with it and it will work as intended.
//
// *****************************************************************************************************************
// *****************************************************************************************************************
//
// Code Modifications:
// Web Ui added with wifi, mqtt and sensor calibration calculations settings
// mqttClientId changed uses last 6 digits of mac address
// Webserial Working with commands to change Wifi, Mqtt and Sensor Calibration Calculations
// MQTT Working
// Display Working with both OLED. Minor issus with text on 0.9 oled
// Hold boot to upload to esp32 if com port fails 
// Added factory reset button functionality (GPIO 13, hold 10s)
// Added historical data charting with 36h/24h/12h views
// Added data storage for sensor readings
// Debug Mode removed. Webserial prints as it goes now.
// Google sheets / script added
//
//
// Future Features:
// More web ui system info. eg. Memorey Dump, wifi strength,
// Wifi Manager intergration
// OTA updates
// Login for web ui
//
//
// *****************************************************************************************************************
// *****************************************************************************************************************
//
// Hardware Required
//
// ESP32-WROOM-32D 32U 30Pin module with breakout board   @ https://www.aliexpress.com/item/1005006422498371.html
// ComWinTop THC-S RS485 Sensor  @https://www.aliexpress.com/item/1005001524845572.html
// OR a compatible TH 3001 (Temp/Humidity) RS485 Sensor
// 1.3" OLED Display Module  @ https://www.aliexpress.com/item/1005006127524245.html
// MAX485 module RS485 module   @ https://www.aliexpress.com/item/1005003204223371.html
// Momentary Push Button for Factory Reset
//
//
// Optional
//
// 2.54mm/0.1" Pitch PCB Screw Terminal Block Connector 4 Pin Terminals   @ https://www.aliexpress.com/item/4000867583795.html
// Micro USB extension Waterproof Cable,USB 2.0 Micro-5pin Male to Female   @ https://www.aliexpress.com/item/1005005507477278.html
// SP13 Waterproof Connector IP68 4 Pin Cable Connectors   @ https://www.aliexpress.com/item/1005003180200877.html
// 0.96 Inch OLED Display Module SSD1306 I2C IIC SPI Serial 128X64  @ https://www.aliexpress.com/item/1005002038436255.html
// Small Case    @https://www.aliexpress.com/item/1005005289338797.html
// USB Cable with charging and data
// Step drill bit
// Soldering iron
//
//
// ComWinTop THC-S Manual https://dl.artronshop.co.th/CWT-RS485-Soil/THC-S%20manual.pdf
//
// *****************************************************************************************************************
// *****************************************************************************************************************

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h> // Use ArduinoJson Version 6 or 7
#include <U8g2lib.h>
#include <Preferences.h>
#include <AsyncWebSocket.h>
#include <LittleFS.h>
#include <time.h>
#include <HTTPClient.h>
#include "html.h" 


// Pin Definitions
#define RX_PIN 17  // SoftwareSerial RX pin for Modbus communication
#define TX_PIN 16  // SoftwareSerial TX pin for Modbus communication
#define RE_PIN 19  // RS485 Read Enable pin (often connected to DE)
#define DE_PIN 18  // RS485 Driver Enable pin
#define FACTORY_RESET_PIN 13 // GPIO pin for factory reset button

// Forward declarations
void factoryReset();
void sendStatus();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocket *webSocket);
void sendSensorData();

// Goolge Sheets Script
const char* googleScriptURL = "https://script.google.com/macros/s/AKfycbxsoFoTwek-ERuPv2Ou94mNhtiYKuwCG4KwBDW40oPF8d2moQYDlVfz62WYn1AhK56kcQ/exec";

// Structure for sensor data storage
struct SensorDataPoint {
    uint32_t timestamp; // 4 bytes for Unix timestamp
    float vwc;          // 4 bytes for Volumetric Water Content (%)
    float pwec;         // 4 bytes for Pore Water Electrical Conductivity (dS/m)
    float temp;         // 4 bytes for Temperature (Celsius)
} __attribute__((packed)); // Ensure structure is packed tightly (16 bytes total)

// Constants for data storage
const char* SENSOR_DATA_FILE = "/sensor_data.bin"; // Filename for storing sensor data in LittleFS
const size_t MAX_RECORDS = 432; // Max records: 36 hours * (60 mins / 5 min interval) = 432
const unsigned long DATA_SAVE_INTERVAL = 300000; // 5 minutes in milliseconds (5 * 60 * 1000)
const char* ntpServer = "pool.ntp.org"; // NTP server for time synchronization
const long gmtOffset_sec = -4 * 3600; // GMT offset in seconds (e.g., -4 hours for EDT)
const int daylightOffset_sec = 3600; // Daylight saving offset in seconds (1 hour)

// WiFi and MQTT settings (with default values)
String ssid = "Pirate";
String password = "Controls";
String mqttServer = "DefaultIPAddress";
int mqttPort = 1883;
String mqttUsername = "DefaultMqttUsername";
String mqttPassword = "DefaultMqttPassword";
String mqttClientId; // Will be set in setup() using MAC address
String mqttTopic = "sensor/thcs/s1"; // Default MQTT topic (change for multiple sensors)
bool mqtt_connected = false; // Flag for MQTT connection status
bool wifi_connected = false; // Flag for WiFi connection status

// MQTT reconnection state
unsigned long lastMqttReconnectAttempt = 0; // Timestamp of the last MQTT reconnect attempt
const long mqttReconnectInterval = 5000; // Try reconnecting every 5 seconds

// Default calibration settings for EC (Electrical Conductivity)
float EC_SLOPE = 1.00;       // Slope for EC calibration
float EC_INTERCEPT = 0.00;   // Intercept for EC calibration
float EC_TEMP_COEFF = 0.00;  // Temperature coefficient for EC correction

// Create instances of necessary libraries
WiFiClient espClient;             // TCP client for MQTT
PubSubClient client(espClient);   // MQTT client
Preferences preferences;          // For storing settings persistently

// Display-related variables
enum DisplayMode {
    VWC_TEMP_PWEC_IP, // Display VWC, Temp, pwEC, and IP address
    EC_RAW_EC         // Display Calibrated EC and Raw EC reading
};

DisplayMode displayMode = VWC_TEMP_PWEC_IP; // Initial display mode
unsigned long lastDisplaySwitchTime = 0;    // Timestamp of the last display mode switch
const unsigned long displayIntervalVWC = 8000; // Time to show VWC/Temp/pwEC/IP screen (8s)
const unsigned long displayIntervalEC = 8000;  // Time to show EC/Raw EC screen (8s)
unsigned long lastDisplayUpdate = 0;        // Timestamp of the last OLED refresh
const long displayUpdateInterval = 1000;    // Update display content every second

// Timing variables for various tasks
unsigned long lastUpdate = 0;           // Timestamp of last WebSocket sensor data update
unsigned long lastStatusUpdate = 0;     // Timestamp of last WebSocket status update
unsigned long lastDataSave = 0;         // Timestamp of last data save to LittleFS
unsigned long lastMqttPublish = 0;      // Timestamp of last MQTT publish
unsigned long lastWifiCheck = 0;        // Timestamp of last WiFi status check
unsigned long startupTime = 0;          // Timestamp when the device started
const long updateInterval = 2000;       // Update web interface (via WebSocket) every 2 seconds
const long statusInterval = 5000;
// --- Google Sheets retry tracking ---
unsigned long lastSheetsAttempt = 0;
const unsigned long sheetsRetryInterval = 60000; // Retry every 60s
int sheetsRetryCount = 0;
const int sheetsMaxRetries = 3;
bool sheetsPendingRetry = false;
       // Update status (WiFi, MQTT) every 5 seconds

// Sensor reading state machine
enum SensorReadState {
    IDLE,               // Waiting for the next read interval
    START_TRANSMISSION, // Sending Modbus request
    WAIT_FOR_RESPONSE,  // Waiting for Modbus response
    PROCESS_RESPONSE    // Processing received Modbus data
};

SensorReadState sensorState = IDLE;         // Current state of the sensor reading process
unsigned long sensorStateStartTime = 0;     // Timestamp when the current sensor state began
unsigned long sensorReadInterval = 5000;    // Read sensor every 5 seconds
unsigned long responseWaitTime = 100;       // Time to wait for Modbus response (ms)

// Sensor data variables
float soil_hum = 0;     // Volumetric Water Content (%)
float soil_temp = 0;    // Soil Temperature (Celsius)
float soil_pw_ec = 0;   // Pore Water EC (dS/m)
float soil_ec = 0;      // Calibrated Bulk EC (uS/cm)
float as_read_ec = 0;   // Raw Bulk EC reading from sensor (uS/cm)

// State variables
bool sensorError = false;   // Flag indicating if there's an error reading the sensor
bool buttonPressed = false; // Flag indicating if the factory reset button is currently pressed
bool useFahrenheit = false; // Flag to display temperature in Fahrenheit (default is Celsius)
unsigned long buttonPressStartTime = 0; // Timestamp when the factory reset button was pressed
const unsigned long FACTORY_RESET_DELAY = 10000; // Hold button for 10 seconds for factory reset

// Modbus communication variables
const int maxConnectionAttempts = 1; // (Currently unused, potential for retry logic)
// Modbus request frame to read humidity, temperature, and EC (registers 0x0000 to 0x0002)
const byte hum_temp_ec[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x03, 0x05, 0xCB};
// Buffer to store the Modbus response (11 bytes expected: Addr, Func, Len, Data(6), CRC(2))
byte sensorResponse[12] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Serial communication instance for Modbus
SoftwareSerial mod(RX_PIN, TX_PIN);

// Web server and WebSocket instances
AsyncWebServer server(80);          // Web server on port 80
AsyncWebSocket ws("/ws");           // WebSocket endpoint for general communication
AsyncWebSocket webui_ws("/webui/ws"); // WebSocket endpoint specifically for the Web UI page

// Initialize OLED display (SH1106, 128x64, I2C)
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// -------------------- Helper Functions --------------------
// Generates a unique MQTT client ID based on the device's MAC address.

String getUniqueClientId(const String& prefix) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[7]; // 6 hex chars + null terminator
    sprintf(macStr, "%02X%02X%02X", mac[3], mac[4], mac[5]);
    return prefix + "_" + String(macStr);
}


// Gets the device uptime in seconds as a string.

String getUptime() {
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    return String(seconds);
}


 // Prints a debug message to WebSerial.

void webDebugPrint(String message, bool alwaysPrint = false) {
    WebSerial.println(message);
}

// Performs a factory reset.

void factoryReset() {
    webDebugPrint("Performing factory reset...", true);
    preferences.begin("wifi_prefs", false); preferences.clear(); preferences.end();
    preferences.begin("mqtt_prefs", false); preferences.clear(); preferences.end();
    preferences.begin("sensor_prefs", false); preferences.clear(); preferences.end();

    webDebugPrint("Formatting LittleFS...", true);
    if (LittleFS.format()) { webDebugPrint("LittleFS formatted successfully.", true); }
    else { webDebugPrint("Error formatting LittleFS.", true); }

    ssid = "Pirate"; password = "Controls";
    mqttServer = "DefaultIPAddress"; mqttPort = 1883;
    mqttUsername = "DefaultMqttUsername"; mqttPassword = "DefaultMqttPassword";
    mqttTopic = "sensor/thcs/s1";
    EC_SLOPE = 1.00; EC_INTERCEPT = 0.00; EC_TEMP_COEFF = 0.00;
    useFahrenheit = false;

    webDebugPrint("Factory reset complete. Rebooting...", true);
    delay(1000);
    ESP.restart();
}

// Handles incoming messages from WebSerial.

void recvMsg(uint8_t *data, size_t len) {
    String command = "";
    for(size_t i=0; i < len; i++) { command += char(data[i]); }
    WebSerial.println("Received command: " + command);

    
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, command);

    if (error) {
        WebSerial.println("Failed to parse JSON command: " + String(error.c_str()));
        return;
    }

    JsonVariantConst cmd_variant = doc["command"];
    if (cmd_variant.isNull()) {
        WebSerial.println("Error: 'command' field missing in JSON.");
        return;
    }
    const char* cmd = cmd_variant.as<const char*>();

     if (strcmp(cmd, "reset") == 0) {
        WebSerial.println("Performing factory reset via WebSerial command...");
        factoryReset();
    } else if (strcmp(cmd, "status") == 0) {
        sendStatus();
        sendSensorData();
    } else if (strcmp(cmd, "reboot") == 0) {
        WebSerial.println("Rebooting device via WebSerial command...");
        delay(500);
        ESP.restart();
    } else if (strcmp(cmd, "getconfig") == 0) {
        
        StaticJsonDocument<512> configDoc; 
        configDoc["ssid"] = ssid;
        configDoc["mqttServer"] = mqttServer;
        configDoc["mqttPort"] = mqttPort;
        configDoc["mqttUsername"] = mqttUsername;
        configDoc["mqttTopic"] = mqttTopic;
        configDoc["ecSlope"] = EC_SLOPE;
        configDoc["ecIntercept"] = EC_INTERCEPT;
        configDoc["ecTempCoeff"] = EC_TEMP_COEFF;
        configDoc["useFahrenheit"] = useFahrenheit;
        

        String configJson;
        serializeJson(configDoc, configJson);
        WebSerial.println("Current Config: " + configJson);

    } else if (strcmp(cmd, "setwifi") == 0) {
        if (doc.containsKey("ssid") && doc.containsKey("password")) {
            
            ssid = doc["ssid"].as<const char*>();
            password = doc["password"].as<const char*>();
            preferences.begin("wifi_prefs", false);
            preferences.putString("ssid", ssid);
            preferences.putString("password", password);
            preferences.end();
            WebSerial.println("WiFi credentials updated. Rebooting to apply...");
            delay(1000);
            ESP.restart();
        } else {
            WebSerial.println("Error: 'ssid' and 'password' required for setwifi.");
        }
    } else if (strcmp(cmd, "setmqttbroker") == 0) {
         if (doc.containsKey("server") && doc.containsKey("port") && doc.containsKey("user") && doc.containsKey("pass") && doc.containsKey("topic")) {
            
            mqttServer = doc["server"].as<const char*>();
            mqttPort = doc["port"].as<int>();
            mqttUsername = doc["user"].as<const char*>();
            mqttPassword = doc["pass"].as<const char*>();
            mqttTopic = doc["topic"].as<const char*>();

            preferences.begin("mqtt_prefs", false);
            preferences.putString("mqttServer", mqttServer);
            preferences.putInt("mqttPort", mqttPort);
            preferences.putString("mqttUsername", mqttUsername);
            preferences.putString("mqttPassword", mqttPassword);
            preferences.putString("topic", mqttTopic);
            preferences.end();

            WebSerial.println("MQTT broker settings updated. Reconnecting...");
            client.disconnect();
            client.setServer(mqttServer.c_str(), mqttPort);
            lastMqttReconnectAttempt = 0;
        } else {
            WebSerial.println("Error: 'server', 'port', 'user', 'pass', and 'topic' required for setmqttbroker.");
        }
    } else if (strcmp(cmd, "setcalibration") == 0) {
         if (doc.containsKey("slope") && doc.containsKey("intercept") && doc.containsKey("tempcoeff")) {
            
            EC_SLOPE = doc["slope"].as<float>();
            EC_INTERCEPT = doc["intercept"].as<float>();
            EC_TEMP_COEFF = doc["tempcoeff"].as<float>();

            preferences.begin("sensor_prefs", false);
            preferences.putFloat("EC_SLOPE", EC_SLOPE);
            preferences.putFloat("EC_INTERCEPT", EC_INTERCEPT);
            preferences.putFloat("EC_TEMP_COEFF", EC_TEMP_COEFF);
            preferences.end();
            WebSerial.println("Sensor calibration settings updated.");
        } else {
            WebSerial.println("Error: 'slope', 'intercept', and 'tempcoeff' required for setcalibration.");
        }
    } else {
        WebSerial.println("Unknown command: " + String(cmd));
    }
}


// -------------------- WebSocket Event Handlers --------------------
// Event handler for the original WebSocket endpoint ("/ws").

void onOriginalWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                       void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            webDebugPrint("WebSocket client #" + String(client->id()) + " connected from " + client->remoteIP().toString());
            sendStatus();
            sendSensorData();
            break;
        case WS_EVT_DISCONNECT:
            webDebugPrint("WebSocket client #" + String(client->id()) + " disconnected");
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len, server);
            break;
        case WS_EVT_PONG:
             webDebugPrint("WebSocket PONG received from client #" + String(client->id()));
            break;
        case WS_EVT_ERROR:
            {
                int errorCode = reinterpret_cast<int>(arg);
                String errorMsg = "";
                if (data && len > 0) { errorMsg = String(reinterpret_cast<const char*>(data), len); }
                webDebugPrint("WebSocket client #" + String(client->id()) + " error(" + String(errorCode) + "): " + errorMsg);
            }
            break;
    }
}

// handler for the Web UI WebSocket endpoint ("/webui/ws").

void onWebuiWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                    void *arg, uint8_t *data, size_t len) {
     switch (type) {
        case WS_EVT_CONNECT:
            webDebugPrint("WebUI WebSocket client #" + String(client->id()) + " connected from " + client->remoteIP().toString());
            sendStatus();
            sendSensorData();
            
            break;
        case WS_EVT_DISCONNECT:
            webDebugPrint("WebUI WebSocket client #" + String(client->id()) + " disconnected");
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len, server);
            break;
        case WS_EVT_PONG:
             webDebugPrint("WebUI WebSocket PONG received from client #" + String(client->id()));
            break;
        case WS_EVT_ERROR:
             {
                int errorCode = reinterpret_cast<int>(arg);
                String errorMsg = "";
                if (data && len > 0) { errorMsg = String(reinterpret_cast<const char*>(data), len); }
                webDebugPrint("WebUI WebSocket client #" + String(client->id()) + " error(" + String(errorCode) + "): " + errorMsg);
            }
            break;
    }
}

// -------------------- Modbus Functions --------------------
// Calculates the CRC16 checksum for Modbus communication.

uint16_t calculateCRC(byte* data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (size_t j = 0; j < 8; j++) {
            if (crc & 0x0001) { crc >>= 1; crc ^= 0xA001; }
            else { crc >>= 1; }
        }
    }
    return crc;
}

// Checks the validity of a received Modbus response.

bool checkModbusResponse(byte* response, size_t responseLength, const byte* request) {
    if (responseLength < 5) { webDebugPrint("Modbus response too short: " + String(responseLength) + " bytes"); return false; }
    if (response[1] != (request[1] & 0x7F)) {
         webDebugPrint("Modbus function code mismatch. Request: 0x" + String(request[1], HEX) + ", Response: 0x" + String(response[1], HEX));
        if (response[1] == (request[1] | 0x80)) { webDebugPrint("Modbus Exception Response Code: 0x" + String(response[2], HEX)); }
        return false;
    }
    if (request[1] == 0x03 && response[2] != (request[5] * 2)) { webDebugPrint("Modbus data length mismatch. Expected: " + String(request[5] * 2) + ", Received: " + String(response[2])); return false; }
    if (responseLength != (3 + response[2] + 2)) { webDebugPrint("Modbus total response length mismatch. Expected: " + String(3 + response[2] + 2) + ", Received: " + String(responseLength)); return false; }

    uint16_t calculated_crc = calculateCRC(response, responseLength - 2);
    uint16_t received_crc = response[responseLength - 2] | (response[responseLength - 1] << 8);

    if (calculated_crc != received_crc) {
        webDebugPrint("Modbus CRC mismatch. Calculated: 0x" + String(calculated_crc, HEX) + ", Received: 0x" + String(received_crc, HEX));
        String receivedBytes = "Received Bytes: ";
        for(size_t i=0; i<responseLength; i++) { receivedBytes += String(response[i], HEX) + " "; }
        webDebugPrint(receivedBytes);
    }
    return (calculated_crc == received_crc);
}

// -------------------- Utility Functions --------------------
// Prints the device's MAC address to WebSerial.

void printMACAddress() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18] = {0};
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    WebSerial.println("MAC Address: " + String(macStr));
}

// -------------------- Data Storage Functions --------------------
// Adds a new sensor data point to the binary file in LittleFS.

void addDataPoint(float vwc, float pwec, float temp) {
     time_t now; time(&now);
    if (now < 1000000000) { webDebugPrint("Time not synced yet. Skipping data point save."); return; }

    SensorDataPoint newData = { .timestamp = (uint32_t)now, .vwc = vwc, .pwec = pwec, .temp = temp };

    File file = LittleFS.open(SENSOR_DATA_FILE, "r+");
    if (!file) {
        webDebugPrint("Creating sensor data file: " + String(SENSOR_DATA_FILE));
        file = LittleFS.open(SENSOR_DATA_FILE, "w+");
         if (!file) { webDebugPrint("Error creating sensor data file!"); return; }
    }

    size_t fileSize = file.size(); size_t recordSize = sizeof(SensorDataPoint); size_t numRecords = fileSize / recordSize;
    webDebugPrint("Saving data point. File size: " + String(fileSize) + ", Records: " + String(numRecords));

    if (numRecords >= MAX_RECORDS) {
        webDebugPrint("Max records reached. Shifting data...");
        SensorDataPoint* buffer = new SensorDataPoint[MAX_RECORDS];
        if (!buffer) { webDebugPrint("Failed to allocate buffer for data shifting!"); file.close(); return; }
        file.seek(recordSize);
        size_t bytesRead = file.read((uint8_t*)buffer, (MAX_RECORDS - 1) * recordSize);
        if (bytesRead != (MAX_RECORDS - 1) * recordSize) { webDebugPrint("Error reading data for shifting!"); }
        else {
            file.seek(0);
            size_t bytesWritten = file.write((uint8_t*)buffer, (MAX_RECORDS - 1) * recordSize);
            if (bytesWritten != (MAX_RECORDS - 1) * recordSize) { webDebugPrint("Error writing shifted data!"); }
            else {
                file.seek((MAX_RECORDS - 1) * recordSize);
                file.write((uint8_t*)&newData, recordSize);
                webDebugPrint("Data shifted and new record added.");
            }
        }
        delete[] buffer;
    } else {
        file.seek(fileSize);
        size_t bytesWritten = file.write((uint8_t*)&newData, recordSize);
         if (bytesWritten != recordSize) { webDebugPrint("Error writing new data point!"); }
         else { webDebugPrint("New data point added."); }
    }
    file.close();
}


// Retrieves historical sensor data from LittleFS within a specified time window.

String getDataJSON(uint32_t hours) {
    File file = LittleFS.open(SENSOR_DATA_FILE, "r");
    if (!file || file.size() == 0) { webDebugPrint("Sensor data file not found or empty."); return "[]"; }

    time_t now; time(&now);
    uint32_t currentTime = (uint32_t)now;
    uint32_t timeThreshold = (hours > 0 && currentTime > (hours * 3600)) ? currentTime - (hours * 3600) : 0;
    webDebugPrint("Getting data for last " + String(hours) + " hours. Threshold: " + String(timeThreshold));

    size_t fileSize = file.size(); size_t numRecords = fileSize / sizeof(SensorDataPoint);
    size_t estimatedJsonSize = 20 + numRecords * 80;
    DynamicJsonDocument jsonDoc(estimatedJsonSize + 2048); // Dynamic allocation ok
    JsonArray dataArray = jsonDoc.to<JsonArray>();

    SensorDataPoint data; size_t recordsAdded = 0;
    while (file.available() >= sizeof(SensorDataPoint)) {
        size_t bytesRead = file.read((uint8_t*)&data, sizeof(SensorDataPoint));
        if (bytesRead != sizeof(SensorDataPoint)) { webDebugPrint("Error reading record from data file."); break; }

        if (data.timestamp >= timeThreshold) {
            JsonObject record = dataArray.createNestedObject();
            if (record.isNull()) { webDebugPrint("Failed to create JSON object for record (JSON capacity exceeded?)."); break; }
            record["t"] = data.timestamp;
            record["vwc"] = float(String(data.vwc, 2).toFloat());
            record["pwec"] = float(String(data.pwec, 2).toFloat());
            record["temp"] = float(String(data.temp, 2).toFloat());
            recordsAdded++;
             if (jsonDoc.overflowed()) {
                webDebugPrint("JSON document overflowed during data population!");
                dataArray.remove(dataArray.size() - 1); recordsAdded--; break;
            }
        }
    }
    file.close();

    String jsonString; serializeJson(jsonDoc, jsonString);
    webDebugPrint("Returning " + String(recordsAdded) + " records. Final JSON size: " + String(jsonString.length()));
     if (jsonDoc.overflowed()) { webDebugPrint("JSON document overflowed after creating chart data!"); return "[]"; }
    webDebugPrint("getDataJSON returning: '" + jsonString + "'");
    return jsonString;
}

// -------------------- WebSocket Communication --------------------
//Sends the current status via WebSocket.

void sendStatus() {
    wifi_connected = (WiFi.status() == WL_CONNECTED);

    StaticJsonDocument<384> doc;
    doc["type"] = "status";
    doc["mqtt_connected"] = mqtt_connected;
    doc["mqtt_topic"] = mqttTopic;
    doc["wifi_connected"] = wifi_connected;
    doc["sensor_error"] = sensorError;
    doc["uptime"] = getUptime();
    
    if (wifi_connected) {
        doc["ssid"] = ssid;
        doc["ip"] = WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
    } else {
         doc["ssid"] = "N/A";
         doc["ip"] = "N/A";
         doc["rssi"] = 0;
    }

    String json;
    serializeJson(doc, json);
    ws.textAll(json);
    webui_ws.textAll(json);
}

// Handles incoming WebSocket messages.

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocket *webSocket) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        StaticJsonDocument<1024> json;
        DeserializationError error = deserializeJson(json, (char*)data);

        if (error) {
            webDebugPrint("Failed to parse WebSocket message: " + String(error.c_str()));
            return;
        }

        JsonVariantConst type_variant = json["type"];
        if (type_variant.isNull()) {
             webDebugPrint("WebSocket message missing 'type' field.");
             return;
        }
        const char* type = type_variant.as<const char*>();

        webDebugPrint("Received WebSocket message type: " + String(type));

        if (strcmp(type, "get_chart_data") == 0) {
            uint32_t hours = json["hours"].as<uint32_t>();
            if (hours == 0) hours = 24; // Default to 24 if not present or zero
            String chartDataJsonString = getDataJSON(hours);

            size_t requiredSize = JSON_OBJECT_SIZE(2) + JSON_STRING_SIZE(chartDataJsonString.length()) + 4096;
            DynamicJsonDocument response(requiredSize);
            response["type"] = "chart_data";
            response["data"] = serialized(chartDataJsonString);

            String responseStr;
            serializeJson(response, responseStr);
            if (response.overflowed()) {
                 webDebugPrint("JSON document overflowed while preparing chart data response!");
                 StaticJsonDocument<128> errorDoc;
                 errorDoc["type"] = "error";
                 errorDoc["message"] = "Chart data too large";
                 String errorJson;
                 serializeJson(errorDoc, errorJson);
                 webSocket->textAll(errorJson);
            } else {
                webDebugPrint("Sending chart data. Size: " + String(responseStr.length()));
                webDebugPrint("Sending chart response: " + responseStr);
                webSocket->textAll(responseStr);
            }
        }
        else if (strcmp(type, "get_config") == 0) {
            webDebugPrint("Received get_config request from WebSocket.");
            StaticJsonDocument<512> configDoc;
            configDoc["type"] = "config_update";

            preferences.begin("wifi_prefs", true);
            configDoc["ssid"] = preferences.getString("ssid", ssid);
            preferences.end();

            preferences.begin("mqtt_prefs", true);
            configDoc["mqttServer"] = preferences.getString("mqttServer", mqttServer);
            configDoc["mqttPort"] = preferences.getInt("mqttPort", mqttPort);
            configDoc["mqttUsername"] = preferences.getString("mqttUsername", mqttUsername);
            configDoc["mqttTopic"] = preferences.getString("topic", mqttTopic);
            preferences.end();

            preferences.begin("sensor_prefs", true);
            configDoc["ecSlope"] = preferences.getFloat("EC_SLOPE", EC_SLOPE);
            configDoc["ecIntercept"] = preferences.getFloat("EC_INTERCEPT", EC_INTERCEPT);
            configDoc["ecTempCoeff"] = preferences.getFloat("EC_TEMP_COEFF", EC_TEMP_COEFF);
            configDoc["sensor_number"] = preferences.getInt("sensorNumber", 1);
            configDoc["read_interval"] = preferences.getULong("readInterval", 5000);
            configDoc["response_wait"] = preferences.getULong("responseWait", 100);
            configDoc["save_interval"] = preferences.getInt("saveInterval", 5);
            configDoc["max_records"] = preferences.getInt("maxRecords", MAX_RECORDS);
            configDoc["gmt_offset"] = preferences.getFloat("gmtOffset", gmtOffset_sec / 3600.0);
            preferences.end();

            configDoc["useFahrenheit"] = useFahrenheit;

            String configJson;
            serializeJson(configDoc, configJson);
            ws.textAll(configJson);
            webui_ws.textAll(configJson);
            webDebugPrint("Sent config_update response.");
        }
        else if (strcmp(type, "update_wifi") == 0) {
             if (json.containsKey("ssid") && json.containsKey("password")) {
                ssid = json["ssid"].as<const char*>();
                password = json["password"].as<const char*>();
                preferences.begin("wifi_prefs", false);
                preferences.putString("ssid", ssid);
                preferences.putString("password", password);
                preferences.end();
                webDebugPrint("WiFi settings updated via WebSocket. Rebooting to apply...");

                StaticJsonDocument<200> response;
                response["type"] = "wifi_update";
                response["success"] = true;
                response["message"] = "WiFi settings saved. Rebooting...";
                String responseJson;
                serializeJson(response, responseJson);
                webSocket->textAll(responseJson);

                delay(1000);
                ESP.restart();
            } else {
                 webDebugPrint("Invalid update_wifi message: missing ssid or password.");
                 StaticJsonDocument<200> response;
                 response["type"] = "wifi_update";
                 response["success"] = false;
                 response["message"] = "Missing SSID or password.";
                 String responseJson;
                 serializeJson(response, responseJson);
                 webSocket->textAll(responseJson);
            }
        }
        else if (strcmp(type, "update_mqtt_topic") == 0) {
            if (json.containsKey("topic")) {
                mqttTopic = json["topic"].as<const char*>();

                preferences.begin("mqtt_prefs", false);
                preferences.putString("topic", mqttTopic);
                preferences.end();
                webDebugPrint("MQTT Topic updated via WebSocket to: " + mqttTopic);

                StaticJsonDocument<256> response;
                response["type"] = "mqtt_topic_update";
                response["success"] = true;
                response["message"] = "MQTT Topic updated successfully.";
                String responseJson;
                serializeJson(response, responseJson);
                webSocket->textAll(responseJson);
                sendStatus();

            } else {
                 webDebugPrint("Invalid update_mqtt_topic message: missing topic field.");
                 StaticJsonDocument<256> response;
                 response["type"] = "mqtt_topic_update";
                 response["success"] = false;
                 response["message"] = "Error: Missing topic field.";
                 String responseJson;
                 serializeJson(response, responseJson);
                 webSocket->textAll(responseJson);
            }
        }
        else if (strcmp(type, "update_mqtt") == 0) {
             if (json.containsKey("server") && json.containsKey("port") && json.containsKey("user") && json.containsKey("pass")) {
                mqttServer = json["server"].as<const char*>();
                mqttPort = json["port"].as<int>();
                mqttUsername = json["user"].as<const char*>();
                mqttPassword = json["pass"].as<const char*>();

                preferences.begin("mqtt_prefs", false);
                preferences.putString("mqttServer", mqttServer);
                preferences.putInt("mqttPort", mqttPort);
                preferences.putString("mqttUsername", mqttUsername);
                preferences.putString("mqttPassword", mqttPassword);
                preferences.end();

                webDebugPrint("MQTT connection settings updated via WebSocket. Reconnecting...");
                client.disconnect();
                client.setServer(mqttServer.c_str(), mqttPort);
                lastMqttReconnectAttempt = 0;

                StaticJsonDocument<256> response;
                response["type"] = "mqtt_update";
                response["success"] = true;
                response["message"] = "MQTT connection settings saved. Reconnecting...";
                String responseJson;
                serializeJson(response, responseJson);
                webSocket->textAll(responseJson);
                sendStatus();
            } else {
                webDebugPrint("Invalid update_mqtt message: missing server, port, user, or pass fields.");
                StaticJsonDocument<256> response;
                response["type"] = "mqtt_update";
                response["success"] = false;
                response["message"] = "Missing required MQTT connection fields (Server, Port, User, Pass).";
                String responseJson;
                serializeJson(response, responseJson);
                webSocket->textAll(responseJson);
            }
        }
        else if (strcmp(type, "update_calibration") == 0) {
            if (json.containsKey("slope") && json.containsKey("intercept") && json.containsKey("tempcoeff")) {
                EC_SLOPE = json["slope"].as<float>();
                EC_INTERCEPT = json["intercept"].as<float>();
                EC_TEMP_COEFF = json["tempcoeff"].as<float>();

                preferences.begin("sensor_prefs", false);
                preferences.putFloat("EC_SLOPE", EC_SLOPE);
                preferences.putFloat("EC_INTERCEPT", EC_INTERCEPT);
                preferences.putFloat("EC_TEMP_COEFF", EC_TEMP_COEFF);
                preferences.end();

                webDebugPrint("Calibration settings updated via WebSocket.");

                StaticJsonDocument<256> response;
                response["type"] = "calibration_update";
                response["success"] = true;
                response["message"] = "Calibration settings saved.";
                String responseJson;
                serializeJson(response, responseJson);
                webSocket->textAll(responseJson);
            } else {
                 webDebugPrint("Invalid update_calibration message: missing fields.");
                 StaticJsonDocument<256> response;
                 response["type"] = "calibration_update";
                 response["success"] = false;
                 response["message"] = "Missing required calibration fields.";
                 String responseJson;
                 serializeJson(response, responseJson);
                 webSocket->textAll(responseJson);
            }
        }
        else if (strcmp(type, "update_single_setting") == 0) {
            const char* setting_type = json["setting_type"];
            if (!setting_type) {
                webDebugPrint("Missing setting type");
                return;
            }

            JsonVariant value = json["value"];
            if (value.isNull()) {
                webDebugPrint("Missing value");
                return;
            }

            bool success = false;
            String settingName = String(setting_type);

            preferences.begin("sensor_prefs", false);
            
            if (settingName == "sensor_number") {
                int sensorNum = value.as<int>();
                if (sensorNum > 0) {
                    preferences.putInt("sensorNumber", sensorNum);
                    success = true;
                    // Update MQTT topic with new sensor number
                    mqttTopic = "sensor/thcs/s" + String(sensorNum);
                    preferences.putString("topic", mqttTopic);
                }
            }
            else if (settingName == "read_interval") {
                int interval = value.as<int>();
                if (interval >= 1000) {
                    sensorReadInterval = interval;
                    preferences.putULong("readInterval", interval);
                    success = true;
                }
            }
            else if (settingName == "response_wait") {
                int wait = value.as<int>();
                if (wait >= 50) {
                    responseWaitTime = wait;
                    preferences.putULong("responseWait", wait);
                    success = true;
                }
            }
            else if (settingName == "save_interval") {
                int saveInt = value.as<int>();
                if (saveInt >= 1) {
                    preferences.putInt("saveInterval", saveInt);
                    success = true;
                }
            }
            else if (settingName == "max_records") {
                int maxRec = value.as<int>();
                if (maxRec >= 1) {
                    preferences.putInt("maxRecords", maxRec);
                    success = true;
                }
            }
            else if (settingName == "gmt_offset") {
                float gmtOff = value.as<float>();
                if (gmtOff >= -12.0 && gmtOff <= 14.0) {  // Valid GMT offset range
                    preferences.putFloat("gmtOffset", gmtOff);
                    configTime(gmtOff * 3600, daylightOffset_sec, ntpServer);
                    success = true;
                }
            }

            preferences.end();

            if (success) {
                StaticJsonDocument<200> response;
                response["type"] = "setting_updated";
                response["setting_type"] = setting_type;
                response["value"] = value;
                String jsonString;
                serializeJson(response, jsonString);
                webSocket->textAll(jsonString);
                webDebugPrint("Setting " + String(setting_type) + " updated to: " + String(value.as<double>()));
            } else {
                webDebugPrint("Invalid setting value for: " + String(setting_type));
                StaticJsonDocument<200> response;
                response["type"] = "error";
                response["message"] = "Invalid setting value";
                String jsonString;
                serializeJson(response, jsonString);
                webSocket->textAll(jsonString);
            }
        }
        else if (strcmp(type, "reboot_device") == 0) {
            webDebugPrint("Reboot requested via WebSocket.");
            
            StaticJsonDocument<200> response;
            response["type"] = "reboot_initiated";
            response["message"] = "Reboot initiated.";
            String responseJson;
            serializeJson(response, responseJson);
            webSocket->textAll(responseJson);
            delay(500);
            ESP.restart();
        }
        else {
             webDebugPrint("Unknown WebSocket message type received: " + String(type));
        }
    }
}

// Sends the latest sensor readings via WebSocket.

void sendSensorData() {
    
    StaticJsonDocument<256> doc;
    doc["type"] = "sensor";
    doc["soil_hum"] = serialized(String(soil_hum, 1));
    doc["soil_temp"] = serialized(String(soil_temp, 1));
    doc["soil_pw_ec"] = serialized(String(soil_pw_ec, 2));
    doc["soil_ec"] = serialized(String(soil_ec, 0));
    doc["raw_ec"] = serialized(String(as_read_ec, 0));
    doc["sensor_error"] = sensorError;

    String json; serializeJson(doc, json);
    ws.textAll(json); webui_ws.textAll(json);
}

// -------------------- Sensor Reading State Machine --------------------
// Manages the non-blocking process of reading data from the Modbus sensor.

void handleSensorReading() {
     unsigned long currentMillis = millis();
    switch(sensorState) {
        case IDLE:
            if (currentMillis - sensorStateStartTime >= sensorReadInterval) {
                sensorState = START_TRANSMISSION; sensorStateStartTime = currentMillis;
                 webDebugPrint("Sensor State: IDLE -> START_TRANSMISSION");
            } break;
        case START_TRANSMISSION:
            digitalWrite(DE_PIN, HIGH); digitalWrite(RE_PIN, HIGH); delayMicroseconds(50);
            while(mod.available()) mod.read();
            webDebugPrint("Sending Modbus request...");
            if (mod.write(hum_temp_ec, sizeof(hum_temp_ec)) == sizeof(hum_temp_ec)) {
                 mod.flush(); delayMicroseconds(100);
                digitalWrite(DE_PIN, LOW); digitalWrite(RE_PIN, LOW);
                sensorState = WAIT_FOR_RESPONSE; sensorStateStartTime = currentMillis;
                 webDebugPrint("Sensor State: START_TRANSMISSION -> WAIT_FOR_RESPONSE");
            } else {
                webDebugPrint("Error sending Modbus request!");
                digitalWrite(DE_PIN, LOW); digitalWrite(RE_PIN, LOW);
                sensorState = IDLE; sensorStateStartTime = currentMillis; sensorError = true; sendStatus();
                 webDebugPrint("Sensor State: START_TRANSMISSION -> IDLE (Send Error)");
            } break;
        case WAIT_FOR_RESPONSE:
            if (mod.available()) {
                int bytesRead = mod.readBytes(sensorResponse, 11);
                if (bytesRead == 11) {
                    sensorState = PROCESS_RESPONSE; webDebugPrint("Sensor State: WAIT_FOR_RESPONSE -> PROCESS_RESPONSE");
                } else {
                    webDebugPrint("Incomplete Modbus response received: " + String(bytesRead) + " bytes. Waiting longer...");
                     if (currentMillis - sensorStateStartTime >= responseWaitTime) {
                        webDebugPrint("Timeout waiting for complete Modbus response.");
                        sensorState = IDLE; sensorStateStartTime = currentMillis; sensorError = true; sendStatus();
                        webDebugPrint("Sensor State: WAIT_FOR_RESPONSE -> IDLE (Timeout)");
                        while(mod.available()) mod.read();
                    }
                }
            } else if (currentMillis - sensorStateStartTime >= responseWaitTime) {
                webDebugPrint("Timeout waiting for Modbus response.");
                sensorState = IDLE; sensorStateStartTime = currentMillis; sensorError = true; sendStatus();
                 webDebugPrint("Sensor State: WAIT_FOR_RESPONSE -> IDLE (Timeout)");
            } break;
        case PROCESS_RESPONSE:
            webDebugPrint("Processing Modbus response...");
            if (checkModbusResponse(sensorResponse, 11, hum_temp_ec)) {
                 webDebugPrint("Modbus response validated successfully.");
                soil_hum = (float)(int16_t)(sensorResponse[3] << 8 | sensorResponse[4]) / 10.0;
                soil_temp = (float)(int16_t)(sensorResponse[5] << 8 | sensorResponse[6]) / 10.0;
                as_read_ec = (float)(uint16_t)(sensorResponse[7] << 8 | sensorResponse[8]);
                soil_ec = EC_SLOPE * as_read_ec + EC_INTERCEPT;
                if (EC_TEMP_COEFF != 0.0 && soil_temp != 25.0) { soil_ec = soil_ec / (1.0 + EC_TEMP_COEFF * (soil_temp - 25.0)); }
                float epsilon_bulk = 1.3088 + 0.1439 * soil_hum + 0.0076 * soil_hum * soil_hum;
                float epsilon_pore = 80.3 - 0.37 * (soil_temp - 20.0);
                float epsilon_offset = 4.1;
                if (epsilon_bulk > epsilon_offset) { float pw_ec_us = (epsilon_pore * soil_ec) / (epsilon_bulk - epsilon_offset); soil_pw_ec = pw_ec_us / 1000.0; }
                else { soil_pw_ec = 0; }
                soil_pw_ec = constrain(soil_pw_ec, 0.0, 20.0);
                if (sensorError) { sensorError = false; sendStatus(); }
                 webDebugPrint("Sensor Read OK: VWC=" + String(soil_hum) + ", Temp=" + String(soil_temp) + ", EC=" + String(soil_ec) + ", pwEC=" + String(soil_pw_ec) + ", RawEC=" + String(as_read_ec));
            } else {
                webDebugPrint("Invalid Modbus Response received.");
                sensorError = true; soil_hum = soil_temp = soil_ec = as_read_ec = soil_pw_ec = 0; sendStatus();
            }
            sensorState = IDLE; sensorStateStartTime = currentMillis;
            webDebugPrint("Sensor State: PROCESS_RESPONSE -> IDLE");
            break;
    }
}


// -------------------- Display Update --------------------
// Handles updating the OLED display content periodically.

void handleDisplayUpdate() {
     unsigned long currentMillis = millis();
    if (currentMillis - lastDisplayUpdate >= displayUpdateInterval) {
        lastDisplayUpdate = currentMillis;
        if (buttonPressed) {
            unsigned long elapsedSeconds = (millis() - buttonPressStartTime) / 1000;
            u8g2.clearBuffer(); u8g2.setFont(u8g2_font_ncenB10_tr);
            u8g2.drawStr(0, 20, "Hold Button"); u8g2.drawStr(0, 40, "for Factory Reset");
            u8g2.setFont(u8g2_font_ncenB14_tr); u8g2.setCursor(50, 60);
            u8g2.print(constrain(FACTORY_RESET_DELAY/1000 - elapsedSeconds, 0, FACTORY_RESET_DELAY/1000)); u8g2.print("s");
            u8g2.sendBuffer(); return;
        }

        if (displayMode == VWC_TEMP_PWEC_IP && (currentMillis - lastDisplaySwitchTime >= displayIntervalVWC)) { displayMode = EC_RAW_EC; lastDisplaySwitchTime = currentMillis; }
        else if (displayMode == EC_RAW_EC && (currentMillis - lastDisplaySwitchTime >= displayIntervalEC)) { displayMode = VWC_TEMP_PWEC_IP; lastDisplaySwitchTime = currentMillis; }

        u8g2.clearBuffer(); u8g2.setFont(u8g2_font_ncenB08_tr);
        char buffer[20];
        switch (displayMode) {
            case VWC_TEMP_PWEC_IP:
                sprintf(buffer, "VWC: %.1f %%", soil_hum); u8g2.drawStr(0, 10, buffer);
                if (useFahrenheit) { sprintf(buffer, "Tmp: %.1f F", (soil_temp * 9.0 / 5.0) + 32.0); }
                else { sprintf(buffer, "Tmp: %.1f C", soil_temp); }
                u8g2.drawStr(0, 25, buffer);
                sprintf(buffer, "pwEC: %.2f dS/m", soil_pw_ec); u8g2.drawStr(0, 40, buffer);
                u8g2.setCursor(0, 55);
                if (wifi_connected) { u8g2.print("IP: "); u8g2.print(WiFi.localIP().toString()); }
                else if (!ssid.isEmpty()) { u8g2.print("Connecting..."); }
                else { u8g2.print("WiFi Not Configured"); }
                break;
            case EC_RAW_EC:
                sprintf(buffer, "EC: %.0f uS/cm", soil_ec); u8g2.drawStr(0, 10, buffer);
                sprintf(buffer, "Raw EC: %.0f", as_read_ec); u8g2.drawStr(0, 25, buffer);
                u8g2.drawStr(0, 40, sensorError ? "Sensor ERROR!" : "Sensor OK");
                u8g2.setCursor(0, 55);
                if (wifi_connected) {
                    u8g2.print("WiFi OK");
                    if (!mqttServer.isEmpty() && mqttServer != "DefaultIPAddress") { u8g2.print(mqtt_connected ? " / MQTT OK" : " / MQTT N/A"); }
                } else { u8g2.print("WiFi N/A"); }
                break;
        }
        u8g2.sendBuffer();
    }
}

// -------------------- Button Handling --------------------
// Checks the state of the factory reset button.

void checkFactoryResetButton() {
     bool currentButtonState = (digitalRead(FACTORY_RESET_PIN) == LOW);
    if (currentButtonState && !buttonPressed) { buttonPressed = true; buttonPressStartTime = millis(); webDebugPrint("Factory reset button pressed.", true); lastDisplayUpdate = 0; }
    else if (currentButtonState && buttonPressed) {
        if (millis() - buttonPressStartTime >= FACTORY_RESET_DELAY) {
            webDebugPrint("Factory reset triggered by button hold.", true);
            u8g2.clearBuffer(); u8g2.setFont(u8g2_font_ncenB10_tr); u8g2.drawStr(10, 35, "Factory Reset!"); u8g2.sendBuffer(); delay(1500);
            factoryReset();
        }
        lastDisplayUpdate = 0;
    } else if (!currentButtonState && buttonPressed) { buttonPressed = false; webDebugPrint("Factory reset button released.", true); lastDisplayUpdate = 0; lastDisplaySwitchTime = millis(); }
}


// -------------------- Network Management --------------------
// Checks the WiFi connection status.

void checkWiFiStatus() {
    unsigned long currentMillis = millis();
    if (currentMillis - lastWifiCheck >= 10000) {
        lastWifiCheck = currentMillis;
        if (WiFi.status() != WL_CONNECTED && !ssid.isEmpty()) {
             if (!wifi_connected) { webDebugPrint("WiFi disconnected. Attempting reconnection...", true); WiFi.disconnect(); WiFi.begin(ssid.c_str(), password.c_str()); }
        } else if (WiFi.status() == WL_CONNECTED && !wifi_connected) {
            webDebugPrint("WiFi reconnected (polled). IP: " + WiFi.localIP().toString(), true);
            wifi_connected = true; sendStatus(); configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        }
    }
}

/**
 * @brief Checks the MQTT connection status.
 */
void checkMqttConnection() {
     if (WiFi.status() != WL_CONNECTED || mqttServer.isEmpty() || mqttServer == "DefaultIPAddress") {
        if (mqtt_connected) { mqtt_connected = false; sendStatus(); } return;
    }
    if (!client.connected()) {
        unsigned long currentMillis = millis();
        if (currentMillis - lastMqttReconnectAttempt >= mqttReconnectInterval) {
            lastMqttReconnectAttempt = currentMillis;
            webDebugPrint("Attempting MQTT connection to " + mqttServer + ":" + String(mqttPort) + "...", true);
            bool result;
            if (mqttUsername.isEmpty()) { webDebugPrint("Connecting without MQTT username/password."); result = client.connect(mqttClientId.c_str()); }
            else { webDebugPrint("Connecting with MQTT username: " + mqttUsername); result = client.connect(mqttClientId.c_str(), mqttUsername.c_str(), mqttPassword.c_str()); }
            if (result) { webDebugPrint("MQTT Connected successfully.", true); mqtt_connected = true; }
            else {
                webDebugPrint("MQTT connection failed, rc=" + String(client.state()) + ". Will retry later.", true);
                switch (client.state()) {
                    case MQTT_CONNECTION_TIMEOUT: webDebugPrint(" MQTT State: Connection timed out", true); break;
                    case MQTT_CONNECTION_LOST: webDebugPrint(" MQTT State: Connection lost", true); break;
                    case MQTT_CONNECT_FAILED: webDebugPrint(" MQTT State: Connect failed", true); break;
                    case MQTT_DISCONNECTED: webDebugPrint(" MQTT State: Disconnected", true); break;
                    case MQTT_CONNECT_BAD_PROTOCOL: webDebugPrint(" MQTT State: Bad protocol", true); break;
                    case MQTT_CONNECT_BAD_CLIENT_ID: webDebugPrint(" MQTT State: Bad client ID", true); break;
                    case MQTT_CONNECT_UNAVAILABLE: webDebugPrint(" MQTT State: Server unavailable", true); break;
                    case MQTT_CONNECT_BAD_CREDENTIALS: webDebugPrint(" MQTT State: Bad credentials", true); break;
                    case MQTT_CONNECT_UNAUTHORIZED: webDebugPrint(" MQTT State: Unauthorized", true); break;
                    default: webDebugPrint(" MQTT State: Unknown error code " + String(client.state()), true); break;
                }
                mqtt_connected = false;
            }
            sendStatus();
        }
    } else { if (!mqtt_connected) { mqtt_connected = true; sendStatus(); } }
}


/**
 * @brief Publishes the latest sensor data via MQTT.
 */
void publishMQTT() {
     if (!client.connected() || !mqtt_connected || mqttTopic.isEmpty() || mqttTopic == "sensor/thcstest") { return; }

    // Use StaticJsonDocument for MQTT payload 
    StaticJsonDocument<256> json; 
    json["vwc"] = serialized(String(soil_hum, 2)); json["temp"] = serialized(String(soil_temp, 2)); json["pwec"] = serialized(String(soil_pw_ec, 2));
    json["ec"] = serialized(String(soil_ec, 0)); json["raw_ec"] = serialized(String(as_read_ec, 0)); json["error"] = sensorError;
    json["ip"] = WiFi.localIP().toString(); json["rssi"] = WiFi.RSSI(); json["uptime_s"] = getUptime().toInt();

    String payload; serializeJson(json, payload);
    webDebugPrint("Publishing MQTT to topic '" + mqttTopic + "': " + payload);
    if (!client.publish(mqttTopic.c_str(), payload.c_str())) { webDebugPrint("MQTT publish failed!"); }
}


// -------------------- Setup Function --------------------


// --- Google Sheets Logging Function ---
void sendToGoogleSheets() {
    if (WiFi.status() == WL_CONNECTED && !sensorError) {
        HTTPClient http;
        http.begin(googleScriptURL);
        http.addHeader("Content-Type", "application/json");

        String payload = "{";
        payload += "\"sensor\":\"THCS-S3 \",";
        payload += "\"vwc\":" + String(soil_hum, 1) + ",";
        payload += "\"pwec\":" + String(soil_pw_ec, 2) + ",";
        payload += "\"temp\":" + String(soil_temp, 1) + ",";
        payload += "}";

        int httpCode = http.POST(payload);
        if (httpCode > 0 && httpCode == 200) {
            WebSerial.println("Google Sheets logging success.");
            sheetsRetryCount = 0;
            sheetsPendingRetry = false;
        } else {
            WebSerial.println("Google Sheets logging failed. HTTP code: " + String(httpCode));
            sheetsPendingRetry = true;
            lastSheetsAttempt = millis();
            sheetsRetryCount++;
        }

        http.end();
    } else {
        WebSerial.println("Skipped Google Sheets logging (WiFi or sensor error).");
        sheetsPendingRetry = true;
        lastSheetsAttempt = millis();
        sheetsRetryCount++;
    }
}
void setup() {
    pinMode(DE_PIN, OUTPUT); pinMode(RE_PIN, OUTPUT); pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
    Serial.begin(115200); while (!Serial);
    Serial.println("\n\n--- Pirate Controls THC-S Sensor v1.3 Booting ---");
    mod.begin(4800);
    digitalWrite(DE_PIN, LOW); digitalWrite(RE_PIN, LOW);
    startupTime = millis();

    Serial.println("Initializing LittleFS...");
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed! Formatting...");
         if (!LittleFS.begin(true)) { Serial.println("LittleFS Initialization failed even after format. Halting."); while (true) delay(1000); }
    } else {
        Serial.println("LittleFS mounted successfully.");
        File root = LittleFS.open("/"); File file = root.openNextFile();
        while(file){ Serial.print("  FILE: "); Serial.print(file.name()); Serial.print("\tSIZE: "); Serial.println(file.size()); file = root.openNextFile(); }
        root.close();
    }

    Serial.println("Initializing OLED display...");
    u8g2.begin(); u8g2.clearBuffer(); u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 15, "Pirate Controls"); u8g2.drawStr(0, 30, "THC-S Sensor v1.3"); u8g2.drawStr(0, 45, "Initializing...");
    u8g2.sendBuffer(); delay(1000);

    Serial.println("Loading preferences...");
    preferences.begin("wifi_prefs", true); ssid = preferences.getString("ssid", ssid); password = preferences.getString("password", password); preferences.end();
    Serial.println(" - WiFi SSID: " + ssid);
    preferences.begin("sensor_prefs", true);
    EC_SLOPE = preferences.getFloat("EC_SLOPE", EC_SLOPE); EC_INTERCEPT = preferences.getFloat("EC_INTERCEPT", EC_INTERCEPT); EC_TEMP_COEFF = preferences.getFloat("EC_TEMP_COEFF", EC_TEMP_COEFF);
    preferences.end();
    Serial.println(" - EC Slope: " + String(EC_SLOPE)); Serial.println(" - EC Intercept: " + String(EC_INTERCEPT)); Serial.println(" - EC Temp Coeff: " + String(EC_TEMP_COEFF));
    preferences.begin("mqtt_prefs", true);
    mqttServer = preferences.getString("mqttServer", mqttServer); mqttPort = preferences.getInt("mqttPort", mqttPort);
    mqttUsername = preferences.getString("mqttUsername", mqttUsername); mqttPassword = preferences.getString("mqttPassword", mqttPassword);
    mqttTopic = preferences.getString("topic", mqttTopic);
    preferences.end();
    Serial.println(" - MQTT Server: " + mqttServer + ":" + String(mqttPort)); Serial.println(" - MQTT Topic: " + mqttTopic);

    Serial.println("Initializing WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        switch(event) {
            case ARDUINO_EVENT_WIFI_STA_START: webDebugPrint("WiFi Station Started.", true); break;
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
                webDebugPrint("WiFi Connected! IP Address: " + WiFi.localIP().toString(), true);
                wifi_connected = true; sendStatus();
                webDebugPrint("Configuring time from NTP server: " + String(ntpServer), true);
                configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
                struct tm timeinfo;
                if(getLocalTime(&timeinfo)){ char timeStr[64]; strftime(timeStr, sizeof(timeStr), "%A, %B %d %Y %H:%M:%S", &timeinfo); webDebugPrint("Current time: " + String(timeStr), true); }
                else { webDebugPrint("Failed to obtain time from NTP.", true); }
                break;
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
                webDebugPrint("WiFi Lost Connection. Reason: " + String(info.wifi_sta_disconnected.reason), true);
                wifi_connected = false; mqtt_connected = false; sendStatus();
                if (!ssid.isEmpty() && ssid != "Pirate") { webDebugPrint("Attempting WiFi reconnection...", true); WiFi.begin(ssid.c_str(), password.c_str()); }
                else { webDebugPrint("WiFi not configured, skipping reconnect attempt.", true); }
                break;
            default: break;
        }
    });

    if (!ssid.isEmpty()) {
         Serial.println("Connecting to WiFi network: " + ssid);
         u8g2.clearBuffer(); u8g2.drawStr(0, 30, "Connecting to:"); u8g2.drawStr(0, 45, ssid.c_str()); u8g2.sendBuffer();
         WiFi.begin(ssid.c_str(), password.c_str());
    } else {
         Serial.println("WiFi SSID is empty. Skipping connection.");
         u8g2.clearBuffer(); u8g2.drawStr(0, 30, "WiFi Not Configured"); u8g2.drawStr(0, 45, "Connect to AP"); u8g2.sendBuffer(); delay(2000);
    }

    mqttClientId = getUniqueClientId("thcs_sensor"); Serial.println("MQTT Client ID: " + mqttClientId);
    client.setServer(mqttServer.c_str(), mqttPort);

    ws.onEvent(onOriginalWsEvent); server.addHandler(&ws);
    webui_ws.onEvent(onWebuiWsEvent); server.addHandler(&webui_ws);

    Serial.println("Setting up web server routes...");
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ Serial.println("Serving /"); request->send_P(200, "text/html", index_html); });
    server.on("/webui", HTTP_GET, [](AsyncWebServerRequest *request){ Serial.println("Serving /webui"); request->send_P(200, "text/html", index_html); });
    
    WebSerial.begin(&server); WebSerial.onMessage(recvMsg); Serial.println("WebSerial initialized.");
    server.begin(); Serial.println("Web server started.");
    printMACAddress(); webDebugPrint("MQTT Client ID: " + mqttClientId, true);
    sensorState = IDLE; sensorStateStartTime = millis() - sensorReadInterval + 1000;
    Serial.println("Setup complete. Starting main loop.");

    u8g2.clearBuffer(); u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 15, "Initialization Done");
    if (WiFi.status() == WL_CONNECTED) { u8g2.drawStr(0, 35, WiFi.localIP().toString().c_str()); }
    else if (!ssid.isEmpty() && ssid != "Pirate") { u8g2.drawStr(0, 35, "Connecting WiFi..."); }
    else { u8g2.drawStr(0, 35, "WiFi Not Configured"); }
    u8g2.drawStr(0, 55, "Running..."); u8g2.sendBuffer(); 
}

// -------------------- Main Loop --------------------

void loop() {
    unsigned long currentMillis = millis();
    checkWiFiStatus(); checkMqttConnection();
    if (client.connected()) { client.loop(); }
    handleSensorReading();
    handleDisplayUpdate();
    if (currentMillis - lastUpdate >= updateInterval) { sendSensorData(); lastUpdate = currentMillis; }
    if (currentMillis - lastStatusUpdate >= statusInterval) { sendStatus(); lastStatusUpdate = currentMillis; }
    ws.cleanupClients(); webui_ws.cleanupClients();
    
if (currentMillis - lastMqttPublish >= sensorReadInterval) {
    publishMQTT();
    lastMqttPublish = currentMillis;

    // --- Google Sheets: initial attempt ---
    sendToGoogleSheets();
}

// --- Google Sheets: retry if needed ---
if (sheetsPendingRetry && sheetsRetryCount <= sheetsMaxRetries) {
    if (millis() - lastSheetsAttempt >= sheetsRetryInterval) {
        WebSerial.println("Retrying Google Sheets logging, attempt #" + String(sheetsRetryCount));
        sendToGoogleSheets();
    }
}

    if (time(nullptr) > 1000000000 && currentMillis - lastDataSave >= DATA_SAVE_INTERVAL) {
        lastDataSave = currentMillis;
        if (!sensorError) { addDataPoint(soil_hum, soil_pw_ec, soil_temp); }
        else { webDebugPrint("Skipping data save due to sensor error."); }
    }
    checkFactoryResetButton();
    // delay(1); // Usually not needed
}
