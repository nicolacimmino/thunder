
#ifndef __THUNDER_TERMINAL_H__
#define __THUNDER_TERMINAL_H__

#include "VT100.h"
#include "messages.h"

#define TERMINAL_WIDTH 80

void printBanner()
{
    VT100.setCursor(2, 1);
    printMessage(0);
}

void printReport(bool thunderstormActive, uint32_t strikes, uint32_t distance, uint32_t energy, uint32_t timeSinceLastStrikeMinutes)
{
    if (!thunderstormActive)
    {
        VT100.setCursor(10, 2);
        VT100.setTextColor(VT_GREEN);
        Serial.print("No Thunderstorm.");
        VT100.cursorOff();
        
        return;
    }

    VT100.setCursor(10, 2);
    VT100.setTextColor(VT_YELLOW);
    Serial.print("STK: ");
    VT100.setTextColor(VT_WHITE);
    Serial.println(strikes);

    VT100.setCursor(12, 2);
    VT100.setTextColor(VT_YELLOW);
    Serial.print("DST: ");
    VT100.setTextColor(VT_WHITE);
    Serial.print(distance);
    Serial.println(" km");

    VT100.setCursor(14, 2);
    VT100.setTextColor(VT_YELLOW);
    Serial.print("ENE: ");
    VT100.setTextColor(VT_WHITE);
    Serial.println(energy);

    VT100.setCursor(16, 2);
    VT100.setTextColor(VT_YELLOW);
    Serial.print("TMS: ");
    VT100.setTextColor(VT_WHITE);
    Serial.print(timeSinceLastStrikeMinutes);
    Serial.println(" min");

    VT100.cursorOff();
}

void printStatusBar()
{
    VT100.setBackgroundColor(VT_BLACK);
    VT100.clearScreen();

    printBanner();

    VT100.setCursor(1, 1);
    VT100.setBackgroundColor(VT_YELLOW);
    VT100.setTextColor(VT_BLACK);
    Serial.print(" Thunder V1.0 ");

    VT100.clearLineAfter();
    VT100.setCursor(1, TERMINAL_WIDTH + 1);
    VT100.setBackgroundColor(VT_BLACK);
    VT100.setTextColor(VT_WHITE);
    VT100.clearLineAfter();
}

#endif