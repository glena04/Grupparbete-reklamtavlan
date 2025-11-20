#include "lcd.h"

HD44780::HD44780()
{
  position_x = 0;
  position_y = 0;
  Initialize();
}

void HD44780::WriteCommand(unsigned char cmd)
{
  OutNibble(cmd >> 4);
  OutNibble(cmd);
  _delay_us(50);
}

void HD44780::WriteData(unsigned char data)
{
  LCD_RS_PORT |= LCD_RS;
  Write(data);
  LCD_RS_PORT &= ~LCD_RS;
}

// UTF-8 aware WriteText function
void HD44780::WriteText(char *text)
{
  int charCount = 0;
  const char *p = text;
  
  while (*p)
  {
    if (charCount == 16) {
      GoTo(0, 1);
    }
    
    unsigned char c = (unsigned char)*p;
    
    // Check for UTF-8 Swedish characters (Ã followed by specific byte)
    if (c == 0xC3 && *(p+1)) {
      uint8_t result;
      unsigned char next = (unsigned char)*(p+1);
      
      switch(next) {
        case 0xA5: result = CHAR_AA_RING; break;  // å
        case 0xA4: result = CHAR_AE_DOTS; break;  // ä
        case 0xB6: result = CHAR_OE_DOTS; break;  // ö
        case 0x85: result = CHAR_AA_RING; break;  // Å
        case 0x84: result = CHAR_AE_DOTS; break;  // Ä
        case 0x96: result = CHAR_OE_DOTS; break;  // Ö
        default: result = '?'; break;
      }
      
      WriteData(result);
      p += 2;  // Skip both UTF-8 bytes
    } else {
      WriteData(c);
      p++;
    }
    
    charCount++;
  }
}

void HD44780::GoTo(unsigned char x, unsigned char y)
{
  unsigned char addr = 0x80 + x + (0x40 * y);
  WriteCommand(addr);
}

void HD44780::Clear(void)
{
  WriteCommand(HD44780_CLEAR);
  _delay_ms(2);
}

void HD44780::Home(void)
{
  WriteCommand(HD44780_HOME);
  _delay_ms(2);
}

void HD44780::Initialize(void)
{
  // Configure pins as output
  LCD_RS_DIR |= LCD_RS;
  LCD_E_DIR |= LCD_E;
  LCD_DB4_DIR |= LCD_DB4;
  LCD_DB5_DIR |= LCD_DB5;
  LCD_DB6_DIR |= LCD_DB6;
  LCD_DB7_DIR |= LCD_DB7;

  LCD_RS_PORT = LCD_RS_PORT & ~(1 << LCD_E) & ~(1 << LCD_RS);

  // Initialization sequence
  _delay_ms(50);
  OutNibble(0x03);
  _delay_ms(5);
  OutNibble(0x03);
  _delay_us(100);
  OutNibble(0x03);
  OutNibble(0x02);

  WriteCommand(0x28); // Function set: 4-bit mode, 2 lines
  WriteCommand(0x0C); // Display ON, Cursor OFF, Blink OFF
  Clear();
  WriteCommand(0x06); // Entry mode: Increment cursor

  _delay_us(25);
  
  // Create custom Swedish characters
  uint8_t aa_ring[8] = {0b00100, 0b00000, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111, 0b00000};
  CreateChar(CHAR_AA_RING, aa_ring);
  
  uint8_t ae_dots[8] = {0b01010, 0b00000, 0b01110, 0b00001, 0b01111, 0b10001, 0b01111, 0b00000};
  CreateChar(CHAR_AE_DOTS, ae_dots);
  
  uint8_t oe_dots[8] = {0b01010, 0b00000, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110, 0b00000};
  CreateChar(CHAR_OE_DOTS, oe_dots);
  
  // Original custom characters for hourglass
  uint8_t customCharTwo[] = hourglass;
  CreateChar(3, customCharTwo);
  uint8_t leftHour[] = leftSideHourglass;
  CreateChar(4, leftHour);
  uint8_t rightHour[] = rightSideHourglass;
  CreateChar(5, rightHour);
}

void HD44780::OutNibble(unsigned char nibble)
{
  if (nibble & 1)
    LCD_DB4_PORT |= LCD_DB4;
  else
    LCD_DB4_PORT &= ~LCD_DB4;
  if (nibble & 2)
    LCD_DB5_PORT |= LCD_DB5;
  else
    LCD_DB5_PORT &= ~LCD_DB5;
  if (nibble & 4)
    LCD_DB6_PORT |= LCD_DB6;
  else
    LCD_DB6_PORT &= ~LCD_DB6;
  if (nibble & 8)
    LCD_DB7_PORT |= LCD_DB7;
  else
    LCD_DB7_PORT &= ~LCD_DB7;

  LCD_E_PORT |= LCD_E;
  _delay_us(1);
  LCD_E_PORT &= ~LCD_E;
}

void HD44780::Write(unsigned char byte)
{
  OutNibble(byte >> 4);
  _delay_ms(5);
  OutNibble(byte);
  _delay_ms(5);
}

#define LCD_SETCGRAMADDR 0x40
void HD44780::CreateChar(uint8_t location, uint8_t charArray[])
{
  location &= 0x7;
  WriteCommand(LCD_SETCGRAMADDR | (location << 3));
  for (uint8_t i = 0; i < 8; i++)
  {
    WriteData(charArray[i]);
  }
}