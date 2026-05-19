#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SparkFunBQ27441.h>
#include <Seeed_HM330X.h>
#include <rpcWiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

using namespace std;

// Serial-Monitor für Debug
#define DEBUG_OUTPUT Serial

WiFiClient wifiClient;
PubSubClient client(wifiClient);
HM330X sensor;
TFT_eSPI tft;
uint8_t buf[30];

const char* SSID     = WLANSSID;
const char* PASS     = WLANPW;
const char* MQTT_CLIENT_ID = "john_sense";
const char MQTT_BROKER_ADRRESS[] = "10.184.196.127";
const int MQTT_PORT = 1883;
const char MQTT_USERNAME[] = "";                      
const char MQTT_PASSWORD[] = "";  
const char PUBLISH_TOPIC[] = "sensordata/particle"; 
const char SUBSCRIBE_TOPIC[] = "sensordata/particle";

const int PUBLISH_INTERVAL = 5000;  // 5 seconds

#define MQTT_MAX_PACKET_SIZE 512

//PMXXX mit XXX = Größe in Mikrometer der Partikel
const char* str[] = {"PM1.0 conc(CF=1,SPM): ",
                     "PM2.5 conc(CF=1,SPM): ",
                     "PM10 conc(CF=1,SPM): ",
                     "PM1.0 conc(AE): ",
                     "PM2.5 conc(AE): ",
                     "PM10 conc(AE): ",
                    };

void print_result(const char* str, uint16_t value, int line) {
    if (NULL == str) {
        return;
    }
    char result[100];
    sprintf(result, "%s %d", str, value);
    tft.drawString(result, 10, 60 + line * 25);
}

/*parse buf with 29 uint8_t-data*/
void parse_result_to_display(uint8_t* data) {
    uint16_t value = 0;
    if (data == NULL) {
        return;
    }
    
    tft.fillRect(10, 60, 310, 180, TFT_BLACK);
    
    for (int i = 1; i <= 6; i++) { 
        /* 
        Konversion in 16 Bit wie folgt:
        Daten pro Datensatz sind 2 Byte. Erstes Byte wird um 8 Stellen links geshifted,
        sodass dies dann mit dem zweiten Byte ORed werden kann um einen 16-Bit Wert zu kriegen.
        */
        value = (uint16_t)data[i * 2] << 8 | data[i * 2 + 1]; 
        print_result(str[i - 1], value, i - 1);
    }
}

/*parse buf with 29 uint8_t-data*/
char* parse_result_to_json(uint8_t* data) {
    static char output[256];

    if (data == NULL) {
        return output;
    }

    JsonDocument doc;
    doc["source"] = "MY_HM330X";
    doc["pm_spm_1_0"] = (uint16_t)data[2] << 8 | data[3];
    doc["pm_spm_2_5"] = (uint16_t)data[4] << 8 | data[5];
    doc["pm_spm_10"] = (uint16_t)data[6] << 8 | data[7];
    doc["pm_ae_1_0"] = (uint16_t)data[8] << 8 | data[9];
    doc["pm_ae_2_5"] = (uint16_t)data[10] << 8 | data[11];
    doc["pm_ae_10"] = (uint16_t)data[12] << 8 | data[13];

    serializeJson(doc, output, sizeof(output));
    return output;
}

void displayInit() {
    // Displayinitialisierung
    // Auflösung: 320 x 240
    tft.begin();
    tft.setRotation(3);
    pinMode(LCD_BACKLIGHT, OUTPUT);
    digitalWrite(LCD_BACKLIGHT, HIGH);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setFreeFont(&FreeMono9pt7b);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    //stuff
  } else {
    //more stuff
  }

}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "WIOClient-";
    clientId += String(random(0xffff), HEX);

    wifiClient.setTimeout(5000);
    client.setSocketTimeout(5);
    
    wifiClient.stop();
    delay(500);

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi lost, reconnecting...");
        WiFi.disconnect();
        delay(500);
        WiFi.begin(SSID, PASS);
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        Serial.println();
    }

    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(PUBLISH_TOPIC, "hello world");
      // ... and resubscribe
      client.subscribe(SUBSCRIBE_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void connect_to_wifi() {
    WiFi.begin(SSID, PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DEBUG_OUTPUT.print(".");
    }
    DEBUG_OUTPUT.print("Connected to: ");
    DEBUG_OUTPUT.println(SSID);
    tft.drawString("Connected to", 10, 20);
    tft.drawString(SSID, 10, 35);
    DEBUG_OUTPUT.println(WiFi.localIP());
}

void setup() {
    DEBUG_OUTPUT.begin(115200);
    delay(1000);
    DEBUG_OUTPUT.println("Boot OK");
    DEBUG_OUTPUT.print("RTL8720 firmware version: ");
    DEBUG_OUTPUT.println(rpc_system_version());
    displayInit();
    tft.drawString("Boot successful!", 10, 5);

    connect_to_wifi();
    delay(2000);

    wifiClient.setTimeout(5000);
    client.setSocketTimeout(5);
    client.setServer(MQTT_BROKER_ADRRESS, MQTT_PORT);
    client.setCallback(callback);

    if (lipo.begin()) {
        unsigned int soc = lipo.soc();
    }
    
    DEBUG_OUTPUT.println("Wio Terminal PM2.5 Sensor Starting...");
    
    // Sensorinitialisierung
    if (sensor.init()) {
        DEBUG_OUTPUT.println("HM330X init failed!!");
        while (1);
    }
    DEBUG_OUTPUT.println("HM330X initialized successfully");

    // Header
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Units: ug/m^3", 10, 5);
    tft.drawString("AE = Atmospheric environment", 10, 20);
    tft.drawString("SPM = Std particulate matter", 10, 35);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    
    DEBUG_OUTPUT.println("Setup complete - starting main loop");

}

void loop() {
    static unsigned long lastUpdate = 0;
    static int updateCount = 0;

    if(!client.connected()) {
        reconnect();
    }
    client.loop();
    
    // Update alle 1000ms
    if (millis() - lastUpdate >= 1000) {
        lastUpdate = millis();
        updateCount++;
        
        DEBUG_OUTPUT.print("Update #");
        DEBUG_OUTPUT.println(updateCount);
        
        // Read sensor data
        if (sensor.read_sensor_value(buf, 29)) {
            DEBUG_OUTPUT.println("HM330X read result failed!!");
        } else {
            // Parse and display the data
            parse_result_to_display(buf);
            DEBUG_OUTPUT.println("Display updated successfully");
            if(client.connected()) {
                client.publish(PUBLISH_TOPIC, parse_result_to_json(buf));
            }
        }
    }
    
    // Refreshrate
    delay(50);
}