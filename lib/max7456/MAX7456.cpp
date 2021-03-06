/*
  Arduino library for MAX7456 video overlay IC

  based on code from Arduino forum members dfraser and zitron
  http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1220054359
  modified/extended by kg4wsv
  gmail: kg4wsv
*/

#include <Arduino.h>
#include "MAX7456.h"


// Constructor
MAX7456::MAX7456() {
    _char_attributes = 0x02;
    _cursor_x = CURSOR_X_MIN;
    _cursor_y = CURSOR_Y_MIN;
}



/* -----------------------------------------------------------------------------
 Private functions - not meant to be used by the user:
  - MAX7456_spi_transfer
  - writeCharLinepos
------------------------------------------------------------------------------ */

// basic SPI transfer: use the Atmega hardware to send and receive one byte
// over SPI. MAX7456_spi_transfer does NOT set chip select, so it is a bit of
// a misnomer: it will do SPI data transfer with whatever SPI device is connected
// and which has its CS set active.
SPISettings spi_settings;

byte MAX7456::MAX7456_spi_transfer(volatile byte data) {
    return SPIClass::transfer(data);
}

void MAX7456::writeCharLinepos(uint8_t c, uint16_t linepos) {
    Poke(DMM_WRITE_ADDR, _char_attributes | 0x40); // enter 8 bit mode, no increment mode
    Poke(DMAH_WRITE_ADDR, linepos>>8); // As linepos cannot be larger than 480, this will clear bit 1, which means we write character index and not the attributes
    Poke(DMAL_WRITE_ADDR, linepos&0xFF);
    Poke(DMDI_WRITE_ADDR, c);
}

/* -----------------------------------------------------------------------------
 Functions related to initialization of the MAX7456, basic settings, hardware
 related stuff...
  - Poke
  - Peek
  - begin
  - reset
  - initialize
  - offset
------------------------------------------------------------------------------ */


void MAX7456::Poke(byte address, byte data) {
    SPIClass::beginTransaction(spi_settings);
    MAX7456_spi_transfer(address);
    MAX7456_spi_transfer(data);
    SPIClass::endTransaction();
}

byte MAX7456::Peek(byte address) {
    byte retval=0;
    SPIClass::beginTransaction(spi_settings);
    MAX7456_spi_transfer(address);
    retval=MAX7456_spi_transfer(0xff);
    SPIClass::endTransaction();
    return(retval);
}

// do a soft reset of the MAX7456
void MAX7456::reset() {
    Poke(VM0_WRITE_ADDR, MAX7456_reset); // soft reset
    delay(1); // datasheet: after 100 us, STAT[6] can be polled to verify that the reset process is complete
    while (Peek(VM0_READ_ADDR) & (1<<1)) delay(1); // wait for RESET bit to be cleared
    initialize();
}

// initialize the default parameters for the MAX7456
// your personal preferences go here
void MAX7456::initialize() {
    Poke(DMM_WRITE_ADDR, 0x40 | _char_attributes); // 8 bit operation mode, default for attribute bits is all off, dont clear memory, no auto increment mode
    // set basic mode: enable, PAL/NTSC, Sync mode, ...
    Poke(VM0_WRITE_ADDR, VERTICAL_SYNC_NEXT_VSYNC|OSD_ENABLE|VIDEO_MODE_NTSC|SYNC_MODE_AUTO);
    // set more basic modes: background mode brightness, blinking time, blinking duty cycle:
    Poke(VM1_WRITE_ADDR, BLINK_DUTY_CYCLE_50_50 | BACKGROUND_BRIGHTNESS_21);
    // set all rows to same character white level, 90%
    for (int x = 0; x < MAX_screen_rows; x++) {
        Poke(x+0x10, WHITE_level_90);
    }
}

void MAX7456::begin() {
    SPIClass::begin();
    // now configure the MAX7456
    reset();
}



// Adjust the horizontal and vertical offset
// Horizontal offset between -32 and +31
// Vertical offset between -15 and +16
void MAX7456::offset(int horizontal, int vertical) {
    //Constrain horizontal between -32 and +31
    if (horizontal < -32) horizontal = -32;
    if (horizontal > 31)  horizontal = 31;

    //Constrain vertical between -15 and +16
    if (vertical < -15) vertical = -15;
    if (vertical > 16)  vertical = 16;

    // Write new offsets to the OSD
    Poke(HOS_WRITE_ADDR,horizontal);
    Poke(VOS_WRITE_ADDR,vertical);

}
/* -----------------------------------------------------------------------------
 Cursor setting functions
  - clear
  - home
  - advanceCursor
  - setCursor
------------------------------------------------------------------------------ */

void MAX7456::clear() {
    Poke(DMM_WRITE_ADDR,CLEAR_display);
    home();
    while(Peek(DMM_READ_ADDR) & CLEAR_display) ; // wait until operation is completed and bit is set to zero again
}

// send the cursor to home
void MAX7456::home() {
    setCursor(CURSOR_X_MIN, CURSOR_Y_MIN);
}
// send the cursor to position (x,y)
void MAX7456::setCursor(uint8_t x, uint8_t y) {
    if (x > CURSOR_X_MAX) x = CURSOR_X_MAX;
    if (y > CURSOR_Y_MAX) y = CURSOR_Y_MAX;
    _cursor_y = y; _cursor_x = x;
}
void MAX7456::advanceCursor() {
    if (++_cursor_x >= CURSOR_X_MAX) {
        if (++_cursor_y >= CURSOR_Y_MAX) _cursor_y = CURSOR_Y_MIN;
        _cursor_x = CURSOR_X_MIN;
    }
}


/* --------------------------------------------------------------------------
   Single character printing primitives
   - writeChar
   - writeCharWithAttributes
   - writeCharXY
   --------------------------------------------------------------------------- */


void MAX7456::writeCharXY(uint8_t c, uint8_t x, uint8_t y) {
    setCursor(x,y);
    writeChar(c);
}

void MAX7456::writeChar(uint8_t c) {
    writeCharLinepos(c, _cursor_y * 30 + _cursor_x);
    advanceCursor(); // compute next cursor position
}

void MAX7456::writeCharWithAttributes(uint8_t c, uint8_t attributes) {
    uint16_t linepos = _cursor_y * 30 + _cursor_x; // convert x,y to line position
    writeCharLinepos(c, linepos);
    Poke(DMAH_WRITE_ADDR, 0x02 | linepos>>8);
    Poke(DMAL_WRITE_ADDR, linepos&0xFF);
    Poke(DMDI_WRITE_ADDR, attributes);
    advanceCursor();
}

// writeString is faster than a sequence of writeChar(). It is slightly slower for
//  strings of 1 char length, but 5 SPI transfers per character faster generally. Note that the string must
//  be null-terminated and must not contain the character 0xFF (this is a restriction of the MAX7456).
//  This function works nicely if CURSOR_X_MIN is zero. If CURSOR_X_MIN is nonzero, any overwrapping write
//  will start at x=0 and not at x=CURSOR_X_MIN, so make sure that the string you print is not longer
//  than the rest of the line.
void MAX7456::writeString(const char c[]) {
    uint16_t i=0;
    uint16_t linepos = _cursor_y * 30 + _cursor_x; // convert x,y to line position

    //disableDisplay();
    Poke(DMAH_WRITE_ADDR, linepos>>8); // As linepos cannot be larger than 480, this will clear bit 1, which means we write character index and not the attributes
    Poke(DMAL_WRITE_ADDR, linepos&0xFF);

    Poke(DMM_WRITE_ADDR, _char_attributes | 0x01); // enter 16 bit mode and auto increment mode

    // the i<480 is for safety, if the user gives us a string without zero at the end
    SPIClass::beginTransaction(spi_settings);
    while(c[i] != 0 && i < 480) {
        MAX7456_spi_transfer(c[i]);
        advanceCursor();
        i++;
    }
    // send 0xFF to end the auto-increment mode
    MAX7456_spi_transfer(0xFF);
    SPIClass::endTransaction();

    Poke(DMM_WRITE_ADDR, _char_attributes | 0x40);   // back to 8 bit mode
    //enableDisplay();
}

// basic, slow writeString method. Honors CURSOR_X_MIN.
void MAX7456::writeStringSlow(const char c[]) {
    uint16_t i=0;
    while(c[i] != 0 && i < 480) {
        writeChar(c[i]);
        i++;
    }
}




/* the following functions set the default mode bits for incremental mode printing of the MAX7456. */
void MAX7456::blink(byte onoff) {
    if (onoff) {
        _char_attributes |= 0x10;
    } else {
        _char_attributes &= ~0x10;
    }
}

void MAX7456::invert(byte onoff) {
    if (onoff) {
        _char_attributes |= 0x08;
    } else {
        _char_attributes &= ~0x08;
    }
}

// Read one character from character memory (x=0..29, y=0..12 (NTSC) or 0..15 (PAL))
byte MAX7456::ReadDisplay(uint16_t x, uint16_t y) {
    byte c;
    uint16_t linepos = y * 30 + x; // convert x,y to line position

    Poke(DMM_WRITE_ADDR,0x40); // 8 bit mode
    Poke(DMAH_WRITE_ADDR, linepos >> 8); // DMAH bit 1 cleared since linepos is <480
    Poke(DMAL_WRITE_ADDR, linepos & 0xFF);
    c=Peek(DMDO_READ_ADDR);
    return(c);
}

void MAX7456::disableDisplay() {
    int8_t vm0 = Peek(VM0_READ_ADDR);
    vm0 &= 0xf7;
    Poke(VM0_WRITE_ADDR, vm0);
}

void MAX7456::enableDisplay() {
    int8_t vm0 = Peek(VM0_READ_ADDR);
    int8_t osdbl = Peek(OSDBL_READ_ADDR);
    vm0 |= 0x80;
    osdbl &= ~0x10;
    Poke(VM0_WRITE_ADDR, vm0);
    Poke(OSDBL_WRITE_ADDR, osdbl);
}

void MAX7456::display(byte onoff) {
    if (onoff) {
        enableDisplay();
    } else {
        disableDisplay();
    }
}








