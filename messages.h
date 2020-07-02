#ifndef __THUNDER_MESSAGES_H__
#define __THUNDER_MESSAGES_H__

#include "VT100.h"

const char message0[] PROGMEM = "\e[33m _______ _                     _           _ \r\n"
                                "|__   __| |                   | |         | |     \e[32mSensor:\e[33m AS3935\r\n"
                                "   | |  | |__  _   _ _ __   __| | ___ _ __| |     \e[32mS/N:\e[33m 202006-002\r\n"
                                "   | |  | '_ \\| | | | '_ \\ / _` |/ _ \\ '__| |     \e[32mAssembled:\e[33m 2020-06-30\r\n"
                                "   | |  | | | | |_| | | | | (_| |  __/ |  |_|     \e[32mBuilt:\e[33m " __DATE__ " " __TIME__ "\r\n"
                                "   |_|  |_| |_|\\__,_|_| |_|\\__,_|\\___|_|  (_)   \r\n"
                                "                          \e[32m(c) Nicola 2020\r\n";

const char *const messages[] PROGMEM = {message0};

void printMessage(uint8_t messageId)
{
    char buffer[512];
    strcpy_P(buffer, (char *)pgm_read_word(&(messages[messageId])));
    Serial.print(buffer);
}

#endif
