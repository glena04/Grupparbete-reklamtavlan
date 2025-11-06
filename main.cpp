#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "lcd.h"
#include "uart.h"

int main(void) {
    // Initialize LCD
    HD44780 lcd;
    lcd.Initialize();
    lcd.Clear();
    
    // Display hello message
    lcd.GoTo(0, 0);
    lcd.WriteText((char*)"Hej! Petter");
    
    // Keep display on
    while(1) {
        // Do nothing, just display
    }
    
    return 0;
}