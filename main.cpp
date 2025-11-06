#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <string.h>
#include "lcd.h"
#include "uart.h"
// NO uart.h - no serial output!

// ============================================
// SVARTE PETTERS SVARTBYGGEN - Clean Version
// NO Serial Output - LCD Only
// ============================================

#define DISPLAY_DURATION_MS 20000  // 20 seconds
#define SCROLL_DELAY_MS 400

// Messages
const char* MESSAGE_EVEN = "Lat Petter bygga at dig";
const char* MESSAGE_ODD = "Bygga svart? Ring Petter";

volatile uint32_t milliseconds = 0;
uint8_t minuteCounter = 0;

ISR(TIMER0_OVF_vect) {
    milliseconds++;
}

void initTimer() {
    TCCR0A = 0;
    TCCR0B = (1 << CS01) | (1 << CS00);
    TIMSK0 = (1 << TOIE0);
    sei();
}

uint32_t millis() {
    uint32_t ms;
    cli();
    ms = milliseconds;
    sei();
    return ms;
}

void displayText(HD44780& lcd, const char* text) {
    lcd.Clear();
    lcd.GoTo(0, 0);
    lcd.WriteText((char*)text);
}

void displayScroll(HD44780& lcd, const char* text, uint32_t duration) {
    uint32_t startTime = millis();
    int textLen = strlen(text);
    int lcdWidth = 16;
    int position = lcdWidth;
    
    while (millis() - startTime < duration) {
        lcd.Clear();
        lcd.GoTo(0, 0);
        
        for (int i = 0; i < lcdWidth; i++) {
            int charIndex = i - position;
            if (charIndex >= 0 && charIndex < textLen) {
                lcd.WriteData(text[charIndex]);
            } else {
                lcd.WriteData(' ');
            }
        }
        
        position--;
        if (position < -textLen) {
            position = lcdWidth;
        }
        
        _delay_ms(SCROLL_DELAY_MS);
    }
}

int main(void) {
    // NO init_serial() - removed!
    
    HD44780 lcd;
    lcd.Initialize();
    lcd.Clear();
    
    initTimer();
    
    // Welcome screen
    displayText(lcd, "Svarte Petters");
    _delay_ms(2000);
    lcd.Clear();
    lcd.GoTo(0, 0);
    lcd.WriteText((char*)"Svartbyggen");
    _delay_ms(2000);
    
    // Main loop
    while(1) {
        bool isEvenMinute = (minuteCounter % 2 == 0);
        
        if (isEvenMinute) {
            // EVEN: Scroll
            displayScroll(lcd, MESSAGE_EVEN, DISPLAY_DURATION_MS);
        } else {
            // ODD: Text
            displayText(lcd, MESSAGE_ODD);
            _delay_ms(DISPLAY_DURATION_MS);
        }
        
        minuteCounter++;
    }
    
    return 0;
}
