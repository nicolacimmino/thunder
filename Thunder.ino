

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
#include <ADCTouch.h>

#define AS3935_ADDR 0x03
#define INDOOR 0x12
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01
#define LEDS_COUNT 10
#define DISPLAY_I2C_ADDRESS 0x3C
#define TOUCH_THRESHOLD 5

SparkFun_AS3935 lightning(AS3935_ADDR);

CRGB led[LEDS_COUNT];

int strikes = 0;
int distance = 0;
int energy = 0;
int interferers = 0;
unsigned long lastStrikeTime = 0;
int timeSinceLastStrikeMinutes = 0;
SSD1306AsciiAvrI2c oled;
int touchRef;
bool thunderstormActive = false;

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

    oled.clear();

    touchRef = ADCTouch.read(A1, 500);
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

char message[160];
char messageSwap[160];

void reportStatus()
{
    static bool displayRawStats = true;
    static unsigned long lastReportTime = 0;

    if (millis() - lastReportTime < 1000)
    {
        return;
    }
    lastReportTime = millis();

    if (ADCTouch.read(A1) - touchRef > TOUCH_THRESHOLD)
    {
        displayRawStats = !displayRawStats;
    }

    if (displayRawStats)
    {
        if (!thunderstormActive)
        {
            sprintf(message, "        Thunder  \n"
                             "                   \n"
                             "STK: %d            \n"
                             "DST: ---            \n"
                             "ENE: ---            \n"
                             "TMS: ---            \n"
                             "                   \n", 0);
        }
        else
        {
            sprintf(message, "        Thunder  \n"
                             "                   \n"
                             "STK: %d            \n"
                             "DST: %d            \n"
                             "ENE: %d            \n"
                             "TMS: %d            \n"
                             "                   \n",
                    strikes, distance, energy, timeSinceLastStrikeMinutes);
        }
    }
    else
    {
        if (!thunderstormActive)
        {
            sprintf(message, "        R.I.P.   \n"
                             "                 \n"
                             "    Storm Thunder \n"
                             "                 \n"
                             "b. 31-12-1970   \n"
                             "d. 21-10-2000\n"
                             "                 \n",
                    min(distance, 30), energy / 2000000, timeSinceLastStrikeMinutes, (strikes / 100) % 10, (strikes / 10) % 10, strikes % 10);
        }
        else
        {
            sprintf(message, "        R.I.P.   \n"
                             "                 \n"
                             "    Storm Thunder \n"
                             "                 \n"
                             "b. %02d-%02d-19%02d  \n"
                             "d. 2%d-0%d-197%d\n"
                             "                 \n",
                    min(distance, 30), energy / 2000000, timeSinceLastStrikeMinutes, (strikes / 100) % 10, (strikes / 10) % 10, strikes % 10);
        }
    }

    if (strcmp(message, messageSwap) != 0)
    {
        oled.setCursor(0, 1);
        oled.print(message);
        memcpy(messageSwap, message, 160);
    }
}

void updateStatusColor()
{
    // Keep breathing! See Sean Voisen great post from which I grabbed the formula.
    // https://sean.voisen.org/blog/2011/10/breathing-led-with-arduino/
    float val = (exp(sin(millis() / 2000.0 * PI)) - 0.36787944) * 108.0;

    if (!thunderstormActive)
    {
        CRGB noStormColor = CRGB::Green;
        noStormColor.fadeToBlackBy(255 - val);

        for (int ix = 0; ix < LEDS_COUNT; ix++)
        {
            led[ix] = noStormColor;
        }
        FastLED.show();

        return;
    }

    timeSinceLastStrikeMinutes = 1 + floor(((millis() - lastStrikeTime) / 60000));

    if (timeSinceLastStrikeMinutes > 90)
    {
        strikes = 0;
        thunderstormActive = false;

        return;
    }

    CRGB color = CHSV(timeSinceLastStrikeMinutes, 255, 255);
    color.fadeToBlackBy(255 - val);

    for (int ix = 0; ix < LEDS_COUNT; ix++)
    {
        led[ix] = color;
    }
    FastLED.show();
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
            thunderstormActive = true;
        }
        while (lightning.readInterruptReg())
        {
            delay(1);
        }
    }

    int controlValue = Serial.read();
    if (controlValue == 't')
    {
        strikes++;
        energy = random(100, 2000000);
        distance = random(1, 20);

        lastStrikeTime = millis();
        lightningShow();
        thunderstormActive = true;
    }

    if (controlValue == 'e')
    {
        strikes = 0;
        thunderstormActive = false;
    }
}
