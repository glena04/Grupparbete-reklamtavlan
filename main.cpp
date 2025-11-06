#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "lcd.h"
#include "uart.h"

int main(void) {
    // Initialize serial communication
    init_serial();
    
    // Initialize LCD
    HD44780 lcd;
    lcd.Initialize();
    lcd.Clear();
    
    printf("System started!\n");
    printf("Svarte Petters Svartbyggen\n");
    
    
    lcd.GoTo(0, 0);
    lcd.WriteText((char*)"Svarte Petters");
    
    printf("LCD initialized\n");
    
    // Keep display on
    while(1) {
        // Do nothing
    }
    
    return 0;
}