#include <cstdlib>
#include <iostream>

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SparkFunBQ27441.h>
#include <Seeed_HM330X.h>
#include <rpcWiFi.h>
#include <PubSubClient.h>

// Serial-Monitor für Debug
#define DEBUG_OUTPUT Serial

WiFiClient wifiClient;
PubSubClient client;
HM330X sensor;
TFT_eSPI tft;
uint8_t buf[30];

const char* ssid     = WLANSSID;
const char* password = WLANPW;
const char* clientID = "john_sense";

//PMXXX mit XXX = Größe in Mikrometer der Partikel
const char* str[] = {"PM1.0 conc(CF=1,SPM): ",
                     "PM2.5 conc(CF=1,SPM): ",
                     "PM10 conc(CF=1,SPM): ",
                     "PM1.0 conc(AE): ",
                     "PM2.5 conc(AE): ",
                     "PM10 conc(AE): ",
                    };

void init_mqtt_client() {
    client.setClient(wifiClient);
    client.setServer("185.45.204.21", 1883);
    client.connect(clientID);
}

void print_result(const char* str, uint16_t value, int line) {
    if (NULL == str) {
        return;
    }
    char result[100];
    sprintf(result, "%s %d", str, value);
    tft.drawString(result, 10, 60 + line * 25);
}

/*parse buf with 29 uint8_t-data*/
void parse_result(uint8_t* data) {
    uint16_t value = 0;
    if (NULL == data) {
        return;
    }
    
    tft.fillRect(10, 60, 310, 180, TFT_BLACK);
    
    for (int i = 1; i <= 6; i++) { 
        value = (uint16_t)data[i * 2] << 8 | data[i * 2 + 1];
        print_result(str[i - 1], value, i - 1);
    }
}

void setup() {
    DEBUG_OUTPUT.begin(115200);
    init_mqtt_client();
    WiFi.begin(ssid, password);
    DEBUG_OUTPUT.print("Trying to connect to: ");
    DEBUG_OUTPUT.println(ssid);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DEBUG_OUTPUT.print(".");
    }

    unsigned int soc = lipo.soc();
    delay(100);
    DEBUG_OUTPUT.println("Wio Terminal PM2.5 Sensor Starting...");
    
    // Sensorinitialisierung
    if (sensor.init()) {
        DEBUG_OUTPUT.println("HM330X init failed!!");
        while (1);
    }
    DEBUG_OUTPUT.println("HM330X initialized successfully");

    // Displayinitialisierung
    tft.begin();
    tft.setRotation(3);
    pinMode(LCD_BACKLIGHT, OUTPUT);
    digitalWrite(LCD_BACKLIGHT, HIGH);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setFreeFont(&FreeMono9pt7b);

    // Header
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
            parse_result(buf);
            DEBUG_OUTPUT.println("Display updated successfully");
            client.publish("sensordata/particle", "TestPayload");
        }
    }
    
    // Refreshrate
    delay(50);
}