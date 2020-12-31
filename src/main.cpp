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

void set_cursor(char * cmdbuf) {
    char * pcmd = cmdbuf;
    int8_t x = int_arg(&pcmd);
    int8_t y = int_arg(&pcmd);
    maxosd.setCursor(x, y);
}

void loop() {
    char cmd_buf[CMD_BUF_SIZE];
    size_t read = Serial.readBytesUntil(0, cmd_buf, sizeof cmd_buf);
    if (read <= 1) {
        return;
    }
    wdt_reset();

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
    bool err = false;
    switch (cmd_buf[0]) {
        case 'p':  // 'print'
            maxosd.writeString(&cmd_buf[1]);
            break;
        case 'l': // 'locate'
            set_cursor(&cmd_buf[1]);
            break;
        case 'e': // 'echo' / serial test
            Serial.write(&cmd_buf[1]);
            break;
        case 'h': // 'heartbeat' / no-op
            break;
        case 'm': // 'millis' / uptime
            Serial.write(millis());
            break;
        case '?':
            Serial.print(maxosd.Peek(0xa0), HEX);
            break;
        case '':
            break;
        case 'r':
            maxosd.reset();
            break;
        case 'c':
            maxosd.clear();
            break;
        default:
            err = true;
    }
    // cmd_executed
    Serial.write(0xff);
    // return value
    if (err) {
        Serial.write(1);
    } else {
        Serial.write(0);
    }
}


