

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
#include "terminal.h"
#include <avr/wdt.h>

#define AS3935_ADDR 0x03
#define INDOOR 0x12
#define OUTDOOR 0xE
#define LIGHTNING_INT 0x08
#define DISTURBER_INT 0x04
#define NOISE_INT 0x01
#define LEDS_COUNT 10
#define DISPLAY_I2C_ADDRESS 0x3C
#define TOUCH_THRESHOLD 10
#define LAUGHTER_PIN 7

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
bool showOngoing = false;
unsigned long laughUntil = 0;

void setup()
{
    Serial.begin(115200);
    VT100.begin(Serial);

    FastLED.addLeds<WS2812B, 5, GRB>(led, LEDS_COUNT);
    FastLED.setBrightness(255);

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

    // Setup a watchdog interrupt every 64mS.
    cli();
    _WD_CONTROL_REG = (1 << WDCE) | (1 << WDE);
    _WD_CONTROL_REG = (1 << WDIE) | (1 << WDP1);
    sei();

    pinMode(LAUGHTER_PIN, OUTPUT);
    digitalWrite(LAUGHTER_PIN, LOW);
}

bool winterMode = false;

ISR(WDT_vect)
{
    if (showOngoing)
    {
        return;
    }

    timeSinceLastStrikeMinutes = floor(((millis() - lastStrikeTime) / 60000));

    float breathRate = (thunderstormActive && timeSinceLastStrikeMinutes < 5) ? (2000.0 / (float)(min(strikes, 10))) : 2000.0;

    // Keep breathing! See Sean Voisen great post from which I grabbed the formula.
    // https://sean.voisen.org/blog/2011/10/breathing-led-with-arduino/
    float val = (exp(sin(millis() / breathRate * PI)) - 0.36787944) * 108.0;

    if (!thunderstormActive)
    {
        if (winterMode)
        {
            for (int ix = 0; ix < LEDS_COUNT; ix++)
            {
                led[ix] = (ix % 2 == 0) ? CRGB::DarkOrange : CRGB::DarkRed;
                led[ix].fadeToBlackBy(random(120) + 135);
            }
        }
        else
        {
            CRGB noStormColor = CRGB::DarkGreen;
            noStormColor.fadeToBlackBy(255 - val);

            for (int ix = 0; ix < LEDS_COUNT; ix++)
            {
                led[ix] = noStormColor;
            }
        }

        FastLED.show();

        return;
    }

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

void lightningShow()
{
    showOngoing = true;

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

    showOngoing = false;
}

void reportStatus()
{
    char message[160];
    static bool displayRawStats = false;
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
            strcpy(message, "        Thunder  \n"
                            "                   \n"
                            "STK: ---            \n"
                            "DST: ---            \n"
                            "ENE: ---            \n"
                            "TMS: ---            \n"
                            "                   \n");
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
            strcpy(message, "        R.I.P.   \n"
                            "                 \n"
                            "    Storm Thunder \n"
                            "                 \n"
                            "b. 31-12-1970   \n"
                            "d. 21-10-2000\n"
                            "                 \n");
        }
        else
        {
            sprintf(message, "        R.I.P.   \n"
                             "                 \n"
                             "    Storm Thunder \n"
                             "                 \n"
                             "b. %02d-%02d-19%02d  \n"
                             "d. 28-02-2%03d\n"
                             "                 \n",
                    (int)min(distance + 1, 31), (int)min(1 + (energy / 200000), 12), timeSinceLastStrikeMinutes, strikes);
        }
    }

    printStatusBar();

    static uint16_t lastPoorCrc = 0;
    uint16_t poorCrc = 0;
    for (uint8_t ix = 0; ix < strlen(message); ix++)
    {
        poorCrc += message[ix];
    }

    if (poorCrc != lastPoorCrc)
    {
        oled.setCursor(0, 1);
        oled.print(message);
    }

    lastPoorCrc = poorCrc;

    printReport(thunderstormActive, strikes, distance, energy, timeSinceLastStrikeMinutes);
}

void loop()
{
    reportStatus();

    if (millis() > laughUntil)
    {
        digitalWrite(LAUGHTER_PIN, LOW);
    }

    byte intVal = lightning.readInterruptReg();
    if (intVal)
    {
        if (intVal == DISTURBER_INT)
        {
            interferers++;        
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
        digitalWrite(LAUGHTER_PIN, HIGH);
        laughUntil = millis() + 2000;
        lightningShow();
        thunderstormActive = true;
    }

    if (controlValue == 'r')
    {
        strikes = 0;
        thunderstormActive = false;
    }

    if (controlValue == 'w')
    {
        winterMode = !winterMode;        
    }    
}
