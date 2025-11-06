#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "lcd.h"
#include "uart.h"

// Timer counter
volatile uint32_t milliseconds = 0;

// ISR for Timer0 overflow (~1ms)
ISR(TIMER0_OVF_vect) {
    milliseconds++;
}

// Initialize timer
void initTimer() {
    TCCR0A = 0;
    TCCR0B = (1 << CS01) | (1 << CS00);  // Prescaler 64
    TIMSK0 = (1 << TOIE0);                // Enable overflow interrupt
    sei();                                // Enable global interrupts
}

// Get milliseconds
uint32_t millis() {
    uint32_t ms;
    cli();
    ms = milliseconds;
    sei();
    return ms;
}

int main(void) {
    init_serial();
    
    HD44780 lcd;
    lcd.Initialize();
    lcd.Clear();
    
    // Initialize timer
    initTimer();
    
    printf("Timer initialized!\n");
    
    lcd.WriteText((char*)"Timer test");
    
    uint32_t lastTime = 0;
    
    // Test timer by printing every second
    while(1) {
        uint32_t currentTime = millis();
        
        if (currentTime - lastTime >= 1000) {
            printf("Seconds: %lu\n", currentTime / 1000);
            lastTime = currentTime;
        }
    }
    
    return 0;
}