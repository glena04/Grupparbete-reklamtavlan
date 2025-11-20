// lcd.h
#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

// Pin configuration
#define LCD_RS_DIR DDRD
#define LCD_RS_PORT PORTD
#define LCD_RS (1 << PD2)

#define LCD_E_DIR DDRD
#define LCD_E_PORT PORTD
#define LCD_E (1 << PD3)

#define LCD_DB4_DIR DDRD
#define LCD_DB4_PORT PORTD
#define LCD_DB4 (1 << PD4)

#define LCD_DB5_DIR DDRD
#define LCD_DB5_PORT PORTD
#define LCD_DB5 (1 << PD5)

#define LCD_DB6_DIR DDRD
#define LCD_DB6_PORT PORTD
#define LCD_DB6 (1 << PD6)

#define LCD_DB7_DIR DDRD
#define LCD_DB7_PORT PORTD
#define LCD_DB7 (1 << PD7)

// HD44780 commands
#define HD44780_CLEAR 0x01
#define HD44780_HOME 0x02

// Custom character definitions for hourglass
#define hourglass { 0x1f, 0x11, 0xa, 0x4, 0x4, 0xa, 0x11, 0x1f }
#define leftSideHourglass { 0x00, 0x0C, 0x0A, 0x09, 0x0A, 0x0C, 0x00, 0x00 }
#define rightSideHourglass { 0x00, 0x06, 0x0A, 0x12, 0x0A, 0x06, 0x00, 0x00 }

// Character slots for custom Swedish characters
#define CHAR_AA_RING 0   // å/Å
#define CHAR_AE_DOTS 1   // ä/Ä
#define CHAR_OE_DOTS 2   // ö/Ö
// Slots 3-5 reserved for hourglass animations

// HD44780 LCD class
class HD44780 {
public:
  HD44780();
  void WriteCommand(unsigned char cmd);
  void WriteData(unsigned char data);
  void WriteText(char *text);  // Now UTF-8 aware!
  void GoTo(unsigned char x, unsigned char y);
  void Clear(void);
  void Home(void);
  void Initialize(void);
  void CreateChar(uint8_t location, uint8_t charArray[]);

private:
  int position_x;
  int position_y;
  void OutNibble(unsigned char nibble);
  void Write(unsigned char byte);
};

#endif /* LCD_H_ */