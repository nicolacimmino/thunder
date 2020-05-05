

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
#include "messages.h"
#include <SSD1306AsciiAvrI2c.h>

#define AS3935_ADDR 0x03
#define INDOOR 0x12
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01
#define LEDS_COUNT 10
#define DISPLAY_I2C_ADDRESS 0x3C

SparkFun_AS3935 lightning(AS3935_ADDR);

CRGB led[LEDS_COUNT];

int strikes = 0;
int distance = 0;
uint32_t energy = 0;
int interferers = 0;
unsigned long lastStrikeTime = 0;
unsigned long timeSinceLastStrikeMinutes = 0;
SSD1306AsciiAvrI2c oled;

void setup()
{
    Serial.begin(9600);

    FastLED.addLeds<WS2812B, 5, GRB>(led, LEDS_COUNT);
    FastLED.setBrightness(100);

    Wire.begin();
    lightning.begin();
    lightning.resetSettings();
    lightning.setIndoorOutdoor(INDOOR);
    lightning.spikeRejection(2);

    for (int ix = 0; ix < LEDS_COUNT; ix++)
    {
        led[ix] = CRGB::Black;
    }

    oled.begin(&Adafruit128x64, DISPLAY_I2C_ADDRESS);
    oled.setFont(System5x7);
    oled.set1X();

    oled.clear(0, oled.displayWidth(), 0, 0);
    oled.setRow(0);
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

    oled.clear(0, oled.displayWidth(), 0, 0);
    oled.setCursor(0, 1);

    oled.println("        Thunder");
    oled.println("");

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

    oled.print("ENE: ");
    if (strikes > 0)
    {
        oled.println(energy);
    }
    else
    {
        oled.println("---");
    }

    oled.print("TMS: ");
    if (timeSinceLastStrikeMinutes > 0)
    {
        oled.print(timeSinceLastStrikeMinutes);
    }
    else
    {
        oled.print("---");
    }
    oled.println(" min      ");
}

void updateStatusColor()
{
    timeSinceLastStrikeMinutes = 1 + floor(((millis() - lastStrikeTime) / 60000));

    CRGB color = CRGB::Green;

    // Keep breathing! See Sean Voisen great post from which I grabbed the formula.
    // https://sean.voisen.org/blog/2011/10/breathing-led-with-arduino/
    float val = (exp(sin(millis() / 2000.0 * PI)) - 0.36787944) * 108.0;

    if (lastStrikeTime != 0 && timeSinceLastStrikeMinutes < 60)
    {
        color = CHSV(timeSinceLastStrikeMinutes, 255, 255);
    }

    color.fadeToBlackBy(255 - val);

    if (led[7] != color)
    {
        for (int ix = 0; ix < LEDS_COUNT; ix++)
        {
            led[ix] = color;
        }
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
            // lightningShow();
            // lastStrikeTime = millis();
        }
        else if (intVal == LIGHTNING_INT)
        {
            strikes++;
            energy = lightning.lightningEnergy();
            distance = lightning.distanceToStorm();

            lastStrikeTime = millis();
            lightningShow();
        }
        while (lightning.readInterruptReg())
        {
            delay(1);
        }
    }

    if(Serial.read() != -1) {
        lightningShow();
    }
}
