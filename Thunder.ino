

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

#define AS3935_ADDR 0x03
#define INDOOR 0x12
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01
#define LEDS_COUNT 10

SparkFun_AS3935 lightning(AS3935_ADDR);

CRGB led[LEDS_COUNT];

int strikes = 0;
int distance = 0;
int totEnergy = 0;
int interferers = 0;
unsigned long lastStrikeTime = 0;

void setup()
{
    Serial.begin(115200);

    FastLED.addLeds<WS2812B, 5, GRB>(led, LEDS_COUNT);
    FastLED.setBrightness(255);

    Wire.begin();
    lightning.begin();
    lightning.resetSettings();
    lightning.setIndoorOutdoor(INDOOR);
    lightning.spikeRejection(4);

    for (int ix = 0; ix < LEDS_COUNT; ix++)
    {
        led[ix] = CRGB::Black;
    }
}

void lightningShow()
{
    byte previousBrightness = FastLED.getBrightness();

    for (int ix = 0; ix < LEDS_COUNT; ix++)
    {
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

    FastLED.setBrightness(previousBrightness);

    for (int ix = 0; ix < LEDS_COUNT; ix++)
    {
        led[ix] = CRGB::Black;
    }
    FastLED.show();
}

void reportStatus()
{
    static unsigned long lastReport = 0;

    if (millis() - lastReport < 1000)
    {
        return;
    }
    lastReport = millis();

    Serial.print("INT: ");
    Serial.println(interferers);

    Serial.print("STK: ");
    Serial.println(strikes);

    Serial.print("DST: ");
    if (strikes > 0)
    {
        Serial.print(distance);
    }
    else
    {
        Serial.print("---");
    }
    Serial.println(" km      ");

    Serial.print("AVE: ");
    if (strikes > 0)
    {
        Serial.println(totEnergy / strikes);
    }
    else
    {
        Serial.println("---");
    }
    Serial.println("------------------------");
}

void updateStatusColor()
{
    byte timeSinceLastStrikeMinutes = ((millis() - lastStrikeTime) / 600);

    CRGB color = CRGB::Black;
    if (timeSinceLastStrikeMinutes < 64)
    {
        color = CHSV(timeSinceLastStrikeMinutes, 255, 255);
    }

    if (led[7] != color)
    {
        led[7] = color;
        FastLED.show();
    }
}

void loop()
{
    reportStatus();
    updateStatusColor();

    byte intVal = lightning.readInterruptReg();
    if (intVal)
    {
        if (intVal == DISTURBER_INT)
        {
            interferers++;
            lightningShow();
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
        while (lightning.readInterruptReg())
        {
            delay(1);
        }
    }
}
