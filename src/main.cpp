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
    Serial.println("OSD_INIT");
    maxosd.begin();
    maxosd.initialize();
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
        case 'p':  // 'print'
            maxosd.writeStringSlow(&cmd_buf[1]);
            break;
        case 'l': // 'locate'
            retcode = set_cursor(&cmd_buf[1]);
            break;
        case 'e': // 'echo' / serial test
            Serial.write(&cmd_buf[1]);
            break;
        case 'h': // 'heartbeat' / no-op
            break;
        case 'm': // 'millis' / uptime
            Serial.print(millis(), DEC);
            //retcode = millis();
            break;
        case 's': // status
            Serial.print(maxosd.Peek(0xa0), HEX);
            //retcode = maxosd.Peek(STAT_READ_ADDR);
            break;
        case 'v': // vm0+vm1 video mode
            Serial.print(maxosd.Peek(VM0_READ_ADDR), HEX);
            Serial.write(' ');
            Serial.print(maxosd.Peek(VM1_READ_ADDR), HEX);
            break;
        case 'r':
            maxosd.reset();
            break;
        case 'c':
            maxosd.clear();
            break;
        default:
            retcode = 0xff;
    }
    // cmd_executed
    Serial.write(0xff);
    // return value
    Serial.write(retcode);
}


