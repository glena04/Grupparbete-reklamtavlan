#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "lcd.h"
#include "uart.h"


// ADVERTISING BILLBOARD - REKLAMTAVLAN
// ============================================
// Requirements:
// - 5 customers with different payment amounts
// - Display time: 20 seconds per message
// - Weighted random selection (payment ratio)
// - NEVER same customer twice in a row
// - Different display modes: text, scroll, blink
// ============================================

#define DISPLAY_DURATION_MS 20000  // 20 seconds
#define SCROLL_DELAY_MS 400        // Scroll speed
#define BLINK_DELAY_MS 500         // Blink speed

// Customer IDs
typedef enum {
    HARRY = 0,      // Hederlige Harrys Bilar - 5000 kr
    FARMOR_ANKA,    // Farmor Ankas Pajer AB - 3000 kr
    PETTER,         // Svarte Petters Svartbyggen - 1500 kr
    LANGBEN,        // Långbens detektivbyrå - 4000 kr
    IOT_REKLAM      // IOT:s Reklambyrå - 1000 kr
} Customer;

// Display modes
typedef enum {
    MODE_TEXT,
    MODE_SCROLL,
    MODE_BLINK
} DisplayMode;

// Message structure
typedef struct {
    const char* text;
    DisplayMode mode;
    Customer customer;
} Message;

// All advertising messages
Message messages[] = {
    // Hederlige Harrys Bilar (5000 kr) - 3 messages, random
    {"Kop bil hos Harry", MODE_SCROLL, HARRY},
    {"God bilaffar Harry", MODE_TEXT, HARRY},
    {"Hederlige Harrys", MODE_BLINK, HARRY},
    
    // Farmor Ankas Pajer AB (3000 kr) - 2 messages, random
    {"Kop paj hos Anka", MODE_SCROLL, FARMOR_ANKA},
    {"Skynda! Marten ater", MODE_TEXT, FARMOR_ANKA},
    
    // Svarte Petters Svartbyggen (1500 kr) - 2 messages, time-based
    {"Lat Petter bygga", MODE_SCROLL, PETTER},       // Even minutes
    {"Bygga svart? Ring", MODE_TEXT, PETTER},        // Odd minutes
    
    // Långbens detektivbyrå (4000 kr) - 2 messages, random
    {"Mysterier? Langben", MODE_TEXT, LANGBEN},
    {"Langben fixar det", MODE_TEXT, LANGBEN},
    
    // IOT:s Reklambyrå (1000 kr) - 1 message
    {"Synas har? IOTs", MODE_TEXT, IOT_REKLAM}
};

// Payment amounts (determines weight in random selection)
const int customerPayments[] = {
    5000,  // HARRY
    3000,  // FARMOR_ANKA
    1500,  // PETTER
    4000,  // LANGBEN
    1000   // IOT_REKLAM
};

const int NUM_MESSAGES = sizeof(messages) / sizeof(messages[0]);
const int NUM_CUSTOMERS = sizeof(customerPayments) / sizeof(customerPayments[0]);

// Global variables
Customer lastCustomer = IOT_REKLAM;  // Initialize to avoid first collision
uint8_t minuteCounter = 0;           // For Petter's time-based messages

// Timer for millis()
volatile uint32_t milliseconds = 0;

// ISR for Timer0 overflow (~1ms)
ISR(TIMER0_OVF_vect) {
    milliseconds++;
}

// Initialize timer for millis() function
void initTimer() {
    TCCR0A = 0;
    TCCR0B = (1 << CS01) | (1 << CS00);  // Prescaler 64
    TIMSK0 = (1 << TOIE0);                // Enable overflow interrupt
    sei();                                // Enable global interrupts
}

// Get milliseconds since start
uint32_t millis() {
    uint32_t ms;
    cli();
    ms = milliseconds;
    sei();
    return ms;
}

// ============================================
// CUSTOMER SELECTION
// ============================================

// Select random customer based on payment weight
// NEVER selects the same customer twice in a row
Customer selectRandomCustomer() {
    // Calculate total weight excluding last customer
    int totalWeight = 0;
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        if ((Customer)i != lastCustomer) {
            totalWeight += customerPayments[i];
        }
    }
    
    // Generate random value
    int randomValue = rand() % totalWeight;
    int cumulativeWeight = 0;
    
    // Find customer based on weighted probability
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        if ((Customer)i != lastCustomer) {
            cumulativeWeight += customerPayments[i];
            if (randomValue < cumulativeWeight) {
                return (Customer)i;
            }
        }
    }
    
    return HARRY;  // Fallback (should never happen)
}

// Get message index for a specific customer
int getMessageForCustomer(Customer customer) {
    // Find all messages for this customer
    int messageIndices[10];
    int count = 0;
    
    for (int i = 0; i < NUM_MESSAGES; i++) {
        if (messages[i].customer == customer) {
            messageIndices[count++] = i;
        }
    }
    
    // Special case: Petter's time-based selection
    if (customer == PETTER) {
        // Even minute (0,2,4...): scroll message
        // Odd minute (1,3,5...): text message
        return messageIndices[minuteCounter % 2];
    }
    
    // Random selection for other customers
    if (count > 0) {
        return messageIndices[rand() % count];
    }
    
    return 0;  // Fallback
}

// ============================================
// DISPLAY FUNCTIONS
// ============================================

// Display static text
void displayText(HD44780& lcd, const char* text) {
    lcd.Clear();
    lcd.GoTo(0, 0);
    lcd.WriteText((char*)text);
}

// Display scrolling text (left scroll)
void displayScroll(HD44780& lcd, const char* text, uint32_t duration) {
    uint32_t startTime = millis();
    int textLen = strlen(text);
    int lcdWidth = 16;
    
    // Start position (off-screen right)
    int position = lcdWidth;
    
    while (millis() - startTime < duration) {
        lcd.Clear();
        lcd.GoTo(0, 0);
        
        // Display visible portion of text
        for (int i = 0; i < lcdWidth; i++) {
            int charIndex = i - position;
            if (charIndex >= 0 && charIndex < textLen) {
                lcd.WriteData(text[charIndex]);
            } else {
                lcd.WriteData(' ');
            }
        }
        
        // Move text left
        position--;
        
        // Reset when text scrolled off screen
        if (position < -textLen) {
            position = lcdWidth;
        }
        
        _delay_ms(SCROLL_DELAY_MS);
    }
}

// Display blinking text
void displayBlink(HD44780& lcd, const char* text, uint32_t duration) {
    uint32_t startTime = millis();
    bool visible = true;
    
    while (millis() - startTime < duration) {
        if (visible) {
            lcd.Clear();
            lcd.GoTo(0, 0);
            lcd.WriteText((char*)text);
        } else {
            lcd.Clear();
        }
        
        visible = !visible;
        _delay_ms(BLINK_DELAY_MS);
    }
}

// Display a message based on its mode
void displayMessage(HD44780& lcd, const Message& msg) {
    printf("Display: %s (mode: %d, customer: %d)\n", 
           msg.text, msg.mode, msg.customer);
    
    switch (msg.mode) {
        case MODE_TEXT:
            displayText(lcd, msg.text);
            _delay_ms(DISPLAY_DURATION_MS);
            break;
            
        case MODE_SCROLL:
            displayScroll(lcd, msg.text, DISPLAY_DURATION_MS);
            break;
            
        case MODE_BLINK:
            displayBlink(lcd, msg.text, DISPLAY_DURATION_MS);
            break;
    }
}


// MAIN PROGRAM


int main(void) {
    // Initialize serial communication
    init_serial();
    
    // Initialize LCD
    HD44780 lcd;
    lcd.Initialize();
    lcd.Clear();
    
    // Initialize timer for millis()
    initTimer();
    
    // Seed random number generator
    srand(TCNT0);
    
    printf("=================================\n");
    printf("REKLAMTAVLAN - Advertising Board\n");
    printf("=================================\n");
    printf("Customers: 5\n");
    printf("Display time: 20 seconds\n");
    printf("System started!\n\n");
    
    // Welcome message
    displayText(lcd, "IOT Reklambyraa");
    _delay_ms(3000);
    lcd.Clear();
    displayText(lcd, "Reklamtavlan");
    _delay_ms(2000);
    
    // Main loop
    while(1) {
        // Select next customer (weighted random, never same twice)
        Customer nextCustomer = selectRandomCustomer();
        
        // Get message for selected customer
        int messageIndex = getMessageForCustomer(nextCustomer);
        Message currentMessage = messages[messageIndex];
        
        // Debug output
        printf("\n--- New Message ---\n");
        printf("Customer: ");
        switch(nextCustomer) {
            case HARRY: printf("Hederlige Harrys Bilar\n"); break;
            case FARMOR_ANKA: printf("Farmor Ankas Pajer AB\n"); break;
            case PETTER: printf("Svarte Petters Svartbyggen\n"); break;
            case LANGBEN: printf("Langbens detektivbyra\n"); break;
            case IOT_REKLAM: printf("IOT:s Reklambyraa\n"); break;
        }
        printf("Payment: %d kr\n", customerPayments[nextCustomer]);
        
        // Display the message
        displayMessage(lcd, currentMessage);
        
        // Update state
        lastCustomer = nextCustomer;
        minuteCounter++;  // Simulate minute counter for Petter
        
        printf("Display complete.\n");
    }
    
    return 0;
}





















