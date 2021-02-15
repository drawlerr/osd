#include <Arduino.h>
#include "util.h"
#include "MAX7456.h"
#include <avr/wdt.h>
#include "errno.h"


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
    Serial.print(F("OSD_INIT"));
    Serial.write(0);
    maxosd.begin();
    maxosd.initialize();
    maxosd.offset(OSD_H_OFFSET, OSD_V_OFFSET);
}

int set_cursor(char * cmdbuf) {
    char * pcmd = cmdbuf;
    int8_t x = int_arg(&pcmd);
    if (errno) {
        return errno;
    }
    int8_t y = int_arg(&pcmd);
    if (errno) {
        return errno;
    }
    if (x < CURSOR_X_MIN || x > CURSOR_X_MAX ||
        y < CURSOR_Y_MIN || y > CURSOR_Y_MAX) {
        Serial.printf(F("ERANGE: x=%d, y=%d"), x, y);
        return ERANGE;
    }
    maxosd.setCursor(x, y);
    return 0;
}

int set_offset(char * cmdbuf) {
    char * pcmd = cmdbuf;
    int8_t x = int_arg(&pcmd);
    if (errno) {
        return errno;
    }
    int8_t y = int_arg(&pcmd);
    if (errno) {
        return errno;
    }

    maxosd.offset(x, y);
    return 0;
}


int set_attr(char * cmdbuf) {
    char attr = cmdbuf[0];
    char * pcmd = &cmdbuf[1];
    int8_t enable = int_arg(&pcmd);
    if (errno) {
        return errno;
    }
    switch (attr) {
        case 'b':
            maxosd.blink(enable);
            break;
        case 'i':
            maxosd.invert(enable);
            break;
        case 'd':
            maxosd.display(enable);
            break;
        default:
            return EDOM;
    }
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
     * a (attr) [0|1] - set boolean attribute
     */
    int retcode = 0;
    switch (cmd_buf[0]) {
        //MAX7456 OSD functions
        case 'p':  // 'print'
            maxosd.writeStringSlow(&cmd_buf[1]);
            break;
        case 'P':  // 'print'
            maxosd.writeString(&cmd_buf[1]);
            break;
        case 'l': // 'locate x y'
            retcode = set_cursor(&cmd_buf[1]);
            break;
        case 'a':
            retcode = set_attr(&cmd_buf[1]);
            break;
        case 's': // status
            Serial.print(maxosd.Peek(STAT_READ_ADDR), HEX);
            break;
        case 'o':
            retcode = set_offset(&cmd_buf[1]);
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


