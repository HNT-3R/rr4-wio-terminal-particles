#include <Seeed_HM330X.h>

/*
    basic_demo.ino
    Example for Seeed PM2.5 Sensor(HM300)

    Copyright (c) 2018 Seeed Technology Co., Ltd.
    Website    : www.seeed.cc
    Author     : downey
    Create Time: August 2018
    Change Log :

    The MIT License (MIT)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#include <Seeed_HM330X.h>
#include"TFT_eSPI.h"



#ifdef  ARDUINO_SAMD_VARIANT_COMPLIANCE
    #define SERIAL_OUTPUT tft
#else
    #define SERIAL_OUTPUT tft
#endif

HM330X sensor;
TFT_eSPI tft;
uint8_t buf[30];

//PMXXX mit XXX = Größe in Mikrometer der Partikel
const char* str[] = {"sensor num: ", "PM1.0 conc(CF=1,SPM): ",
                     "PM2.5 conc(CF=1,SPM): ",
                     "PM10 conc(CF=1,SPM): ",
                     "PM1.0 conc(AE): ",
                     "PM2.5 conc(AE): ",
                     "PM10 conc(AE): ",
                    };


HM330XErrorCode print_result(const char* str, uint16_t value, int line) {
    if (NULL == str) {
        return ERROR_PARAM;
    }
    //Aufloesung: 320 x 240
    char result[100];
    sprintf(result, "%s %d", str, value);
    SERIAL_OUTPUT.drawString("Units: ug/m^3", 10, 5);
    SERIAL_OUTPUT.drawString("AE = Atmospheric environment", 10, 20);
    SERIAL_OUTPUT.drawString("SPM = Standard particulate matter", 10, 35);
    SERIAL_OUTPUT.drawString(result, 10, 40 + line*20);
    return NO_ERROR;
}

/*parse buf with 29 uint8_t-data*/
HM330XErrorCode parse_result(uint8_t* data) {
    uint16_t value = 0;
    if (NULL == data) {
        return ERROR_PARAM;
    }
    for (int i = 1; i < 8 + 4; i++) {
        value = (uint16_t) data[i * 2] << 8 | data[i * 2 + 1];
        print_result(str[i - 1], value, i);

    }

    return NO_ERROR;
}

HM330XErrorCode parse_result_value(uint8_t* data) {
    if (NULL == data) {
        return ERROR_PARAM;
    }
    for (int i = 0; i < 28; i++) {
        //SERIAL_OUTPUT.print(data[i], HEX);
        //SERIAL_OUTPUT.print("  ");
        if ((0 == (i) % 5) || (0 == i)) {
            //SERIAL_OUTPUT.println("");
        }
    }
    uint8_t sum = 0;
    for (int i = 0; i < 28; i++) {
        sum += data[i];
    }
    if (sum != data[28]) {
        //SERIAL_OUTPUT.println("wrong checkSum!!");
    }
    //SERIAL_OUTPUT.println("");
    return NO_ERROR;
}


/*30s*/
void setup() {
    SERIAL_OUTPUT.begin(115200);
    delay(100);
    SERIAL_OUTPUT.println("Serial start");
    if (sensor.init()) {
        SERIAL_OUTPUT.println("HM330X init failed!!");
        while (1);
    }

    tft.begin();
    tft.setRotation(3);
    digitalWrite(LCD_BACKLIGHT, HIGH); // turn on the backlight
    tft.fillScreen(TFT_BLACK);
    tft.setFreeFont(&FreeMono9pt7b);
}


void loop() {
    if (sensor.read_sensor_value(buf, 29)) {
        SERIAL_OUTPUT.println("HM330X read result failed!!");
    }
    parse_result_value(buf);
    parse_result(buf);
    //SERIAL_OUTPUT.println("");
    delay(5000);
}
