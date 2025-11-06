#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "lcd.h"
#include "uart.h"

#define DISPLAY_DURATION_MS 3000

volatile uint32_t milliseconds = 0;

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

// Display static text on LCD
void displayText(HD44780& lcd, const char* text) {
    lcd.Clear();
    lcd.GoTo(0, 0);
    lcd.WriteText((char*)text);
}

int main(void) {
    init_serial();
    
    HD44780 lcd;
    lcd.Initialize();
    lcd.Clear();
    
    initTimer();
    
    printf("Display function test\n");
    
    // Test cycling between two messages
    while(1) {
        displayText(lcd, "Message 1");
        printf("Showing: Message 1\n");
        _delay_ms(DISPLAY_DURATION_MS);
        
        displayText(lcd, "Message 2");
        printf("Showing: Message 2\n");
        _delay_ms(DISPLAY_DURATION_MS);
    }
    
    return 0;
}