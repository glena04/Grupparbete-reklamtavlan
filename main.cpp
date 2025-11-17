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

#define DISPLAY_DURATION_MS 20000  // 20 seconds.
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
    {"Köp bil hos Harry", MODE_SCROLL, HARRY},
    {"God bilaffär Harry", MODE_TEXT, HARRY},
    {"Hederlige Harrys", MODE_BLINK, HARRY},
    
    // Farmor Ankas Pajer AB (3000 kr) - 2 messages, random
    {"Köp paj hos Anka", MODE_SCROLL, FARMOR_ANKA},
    {"Skynda! Mårten äter", MODE_TEXT, FARMOR_ANKA},
    
    // Svarte Petters Svartbyggen (1500 kr) - 2 messages, time-based
    {"Låt Petter bygga", MODE_SCROLL, PETTER},       // Even minutes
    {"Bygga svart? Ring", MODE_TEXT, PETTER},        // Odd minutes
    
    // Långbens detektivbyrå (4000 kr) - 2 messages, random
    {"Mysterier? Långben", MODE_TEXT, LANGBEN},
    {"Långben fixar det", MODE_TEXT, LANGBEN},
    
    // IOT:s Reklambyrå (1000 kr) - 1 message
    {"Synas här? IOTs", MODE_TEXT, IOT_REKLAM}
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

// Initialize timer for millis() function (CTC 1ms)
void initTimer() {
    TCCR0A = (1 << WGM01);                                 // CTC mode
    TCCR0B = (1 << CS01) | (1 << CS00);                    // prescaler 64
    OCR0A = 249;                                           // compare for ~1 ms (16MHz/64/1000-1)
    TIMSK0 = (1 << OCIE0A);                                // enable compare match A interrupt
    sei();                                                 // enable global interrupts
}

// ISR -> increments global milliseconds
ISR(TIMER0_COMPA_vect) {
    milliseconds++;
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

// Helper function for UTF-8 Swedish characters (for displayText and displayScroll)
uint8_t fixSwedishUTF8(const char **text) {
    const unsigned char *p = (const unsigned char*)*text;
    if (p[0]==0xC3 && p[1]!=0) {
        uint8_t result;
        switch(p[1]){
            case 0xA5: result=CHAR_AA_RING; break;  // å
            case 0xA4: result=CHAR_AE_DOTS; break;  // ä
            case 0xB6: result=CHAR_OE_DOTS; break;  // ö
            case 0x85: result=CHAR_AA_RING; break;  // Å
            case 0x84: result=CHAR_AE_DOTS; break;  // Ä
            case 0x96: result=CHAR_OE_DOTS; break;  // Ö
            default: result='?'; break;
        }
        *text += 2;
        return result;
    }
    (*text)++;
    return *p;
}


// ============================================
// DISPLAY FUNCTIONS
// ============================================

// Display static text (UTF-8-aware)
void displayText(HD44780& lcd, const char* text) {
    lcd.Clear();
    lcd.GoTo(0, 0);

    const char *p = text;
    int col = 0;
    while (*p) {
        unsigned char c = (unsigned char)*p;
        if (c == 0xC3) {
            uint8_t out = fixSwedishUTF8(&p);
            lcd.WriteData(out);
        } else {
            lcd.WriteData(c);
            p++;
        }
        col++;
        if (col == 16) {
            lcd.GoTo(0, 1);
        }
    }
}

// Display scrolling text (left scroll) - UTF-8 aware
void displayScroll(HD44780& lcd, const char* text, uint32_t duration) {
    uint32_t startTime = millis();

    // Build array of glyph pointers
    const char *p = text;
    const char *glyphs[128];
    int glyphCount = 0;
    while (*p && glyphCount < (int)sizeof(glyphs)) {
        glyphs[glyphCount++] = p;
        unsigned char c = (unsigned char)*p;
        if (c == 0xC3) p += 2; else p += 1;
    }

    int lcdWidth = 16;
    int pos = lcdWidth;

    while (millis() - startTime < duration) {
        lcd.Clear();
        lcd.GoTo(0, 0);

        for (int col = 0; col < lcdWidth; col++) {
            int gindex = col - pos;
            if (gindex >= 0 && gindex < glyphCount) {
                const char *gp = glyphs[gindex];
                lcd.WriteData(fixSwedishUTF8(&gp));
            } else {
                lcd.WriteData(' ');
            }
        }

        pos--;
        if (pos < -glyphCount) pos = lcdWidth;
        _delay_ms(SCROLL_DELAY_MS);
    }
}

// Display blinking text - NOW uses UTF-8 aware WriteText!
void displayBlink(HD44780& lcd, const char* text, uint32_t duration) {
    uint32_t startTime = millis();
    bool visible = true;
    
    while (millis() - startTime < duration) {
        if (visible) {
            lcd.Clear();
            lcd.GoTo(0, 0);
            lcd.WriteText((char*)text);  // WriteText is now UTF-8 aware!
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


// ============================================
// MAIN PROGRAM
// ============================================

int main(void) {
    // Initialize serial communication
    init_serial();
    
    // Initialize LCD (custom chars are created automatically in Initialize())
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
    displayText(lcd, "IOT Reklambyrå");
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
            case LANGBEN: printf("Långbens detektivbyrå\n"); break;
            case IOT_REKLAM: printf("IOT:s Reklambyrå\n"); break;
        }
        printf("Payment: %d kr\n", customerPayments[nextCustomer]);
        
        // Display the message
        displayMessage(lcd, currentMessage);
        
        // Update state
        lastCustomer = nextCustomer;
        minuteCounter++;
        
        printf("Display complete.\n");
    }
    
    return 0;
}



















