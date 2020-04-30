

//  Copyright (C) 2020 Nicola Cimmino
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include <SPI.h>
#include <Wire.h>
#include "SparkFun_AS3935.h"
#include <FastLED.h>
#include <SSD1306AsciiAvrI2c.h>

#define AS3935_ADDR 0x03
#define INDOOR 0x12
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01
#define DISPLAY_I2C_ADDRESS 0x3C

SparkFun_AS3935 lightning(AS3935_ADDR);

CRGB led[12];
CRGB ledBackup[12];

SSD1306AsciiAvrI2c oled;

int strikes = 0;
int distance = 0;
int totEnergy = 0;
int interferers = 0;
int lastStrikeTime = millis();

// This variable holds the number representing the lightning or non-lightning
// event issued by the lightning detector.
byte intVal = 0;

void setup()
{
    Serial.begin(9600);

    FastLED.addLeds<WS2812B, 5, GRB>(led, 12);
    FastLED.setBrightness(255);

    Wire.begin();
    if (!lightning.begin())
    {
        Serial.println("Lightning Detector did not start up, freezing!");
        while (1)
            ;
    }
    else
    {
        Serial.println("Schmow-ZoW, Lightning Detector Ready!\n");
    }

    lightning.spikeRejection(2);

    for (int ix = 0; ix < 12; ix++)
    {
        led[ix] = CRGB::Black;
    }

    oled.begin(&Adafruit128x64, DISPLAY_I2C_ADDRESS);
    oled.setFont(System5x7);
    oled.set1X();

    oled.clear(0, oled.displayWidth(), 0, 0);
    oled.setRow(0);
}

unsigned long lastShiftTime = 0;

void lightningShow()
{
    byte previousBrightness = FastLED.getBrightness();

    for (int ix = 0; ix < 12; ix++)
    {
        ledBackup[ix] = led[ix];
        led[ix] = CRGB::White;
    }

    for (int l = 0; l < random(4, 12); l++)
    {
        FastLED.setBrightness(random(20, 255));
        FastLED.show();
        delay(random(1, 50));

        FastLED.setBrightness(0);
        FastLED.show();
        delay(random(1, 150));
    }

    FastLED.setBrightness(random(100, 200));
    FastLED.show();
    delay(random(50, 100));

    for (int ix = 100; ix >= 0; ix -= random(1, 5))
    {
        FastLED.setBrightness(ix);
        FastLED.show();
        delay(1);
    }

    FastLED.setBrightness(previousBrightness);
    FastLED.show();

    for (int ix = 0; ix < 12; ix++)
    {
        led[ix] = ledBackup[ix];
    }
}

void loop()
{
    oled.clear(0, oled.displayWidth(), 0, 0);
    oled.setCursor(0, 2);
    oled.print("INT: ");
    oled.println(interferers);

    oled.print("STK: ");
    oled.println(strikes);

    oled.print("DST: ");
    if (strikes > 0)
    {
        oled.print(distance);
    }
    else
    {
        oled.print("---");
    }
    oled.println(" km      ");

    oled.print("AVE: ");
    if (strikes > 0)
    {
        oled.println(totEnergy / strikes);
    }
    else
    {
        oled.println("---");
    }

    intVal = lightning.readInterruptReg();
    if (intVal == NOISE_INT)
    {
        Serial.println("Noise.");
    }
    else if (intVal == DISTURBER_INT)
    {
        interferers++;
        lightningShow();

        if (distance == 0)
        {
            distance = 30;
        }

        distance = distance - 1;
        lastStrikeTime = millis();
    }
    else if (intVal == LIGHTNING_INT)
    {
        strikes++;
        totEnergy += lightning.lightningEnergy();
        distance = lightning.distanceToStorm();

        lastStrikeTime = millis();
        lightningShow();
    }

    byte timeSinceLastStrikeMinutes = ((millis() - lastStrikeTime) / 600);

    CRGB color = CRGB::Black;
    if (timeSinceLastStrikeMinutes < 64)
    {
        color = CHSV(timeSinceLastStrikeMinutes, 255, 255 * (40.0 / distance));
    }

    for (int ix = 0; ix < 12; ix++)
    {
        led[ix] = color;
    }
    FastLED.show();

    delay(2000);
}
