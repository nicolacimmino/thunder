#ifndef __THNDER_MESSAGES_H__
#define __THNDER_MESSAGES_H__

const char message0[] PROGMEM = " _______ _                     _           _ \r\n"
                                "|__   __| |                   | |         | |\r\n"
                                "   | |  | |__  _   _ _ __   __| | ___ _ __| |\r\n"
                                "   | |  | '_ \\| | | | '_ \\ / _` |/ _ \\ '__| |\r\n"
                                "   | |  | | | | |_| | | | | (_| |  __/ |  |_|\r\n"
                                "   |_|  |_| |_|\\__,_|_| |_|\\__,_|\\___|_|  (_)\r\n"
                                "                          \e[32m(c) Nicola 2020\r\n";

const char *const messages[] PROGMEM = {message0};

void messages_print(uint8_t messageId)
{
    char buffer[512];
    strcpy_P(buffer, (char *)pgm_read_word(&(messages[messageId])));
    Serial.print(buffer);
}

#endif
