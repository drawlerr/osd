#include <Arduino.h>
#include "util.h"
#include "MAX7456.h"
#include <avr/wdt.h>


#ifdef DEBUG
#define DEBUG_MSG(...) Serial.printf( __VA_ARGS__ )
#else
#define DEBUG_MSG(...)
#endif


MAX7456 maxosd;

void setup() {
    Serial.setTimeout(SERIAL_TIMEOUT);
    wdt_enable(WDTO_8S);
    Serial.begin(OSD_BAUD);
    Serial.println(F("OSD_INIT"));
    maxosd.begin();
    maxosd.initialize();
    maxosd.offset(OSD_H_OFFSET, OSD_V_OFFSET);
}

int8_t set_cursor(char * cmdbuf) {
    char * pcmd = cmdbuf;
    int8_t row = int_arg(&pcmd);
    int8_t col = int_arg(&pcmd);
    if (col <CURSOR_X_MIN || col > CURSOR_X_MAX ||
        row <CURSOR_Y_MIN || row > CURSOR_Y_MAX) {
        return 0xff;
    }
    maxosd.setCursor(col, row);
    return 0;
}

void loop() {
    char cmd_buf[CMD_BUF_SIZE];

    wdt_reset();

    size_t read = Serial.readBytesUntil(0, cmd_buf, sizeof cmd_buf);
    if (read == 0) {
        return;
    }

    // make sure it's terminated
    if (read <= CMD_BUF_SIZE-1) {
        cmd_buf[read] = '\0';
    } else {
        cmd_buf[CMD_BUF_SIZE-1] = '\0';
    }

    /**
     * p - print string
     * l x y - locate cursor to screen offset
     * r - reset
     * c - clear
     */
    int8_t retcode = 0;
    switch (cmd_buf[0]) {
        //MAX7456 OSD functions
        case 'p':  // 'print'
            maxosd.writeStringSlow(&cmd_buf[1]);
            break;
        case 'l': // 'locate'
            retcode = set_cursor(&cmd_buf[1]);
            break;
        case 'b':
            maxosd.blink_toggle();
            break;
        case 'i':
            maxosd.invert_toggle();
            break;
        case 's': // status
            Serial.print(maxosd.Peek(0xa0), HEX);
            break;
        case 'r':
            maxosd.reset();
            break;
        case 'c':
            maxosd.clear();
            break;
        // serial debug functions
        case 'e': // 'echo' / serial test
            Serial.write(&cmd_buf[1]);
            break;
        case 'm': // 'millis' / uptime
            Serial.print(millis(), DEC);
            break;
        default:
            retcode = 0xff;
    }
    // cmd_executed
    Serial.write(0xff);
    // return value
    Serial.write(retcode);
}


