#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "lcd.h"
#include "uart.h"


// REKLAMTAVLA - ADVERTISING BILLBOARD
// ============================================
// Krav:
// - 5 kunder med olika betalningsbelopp
// - Visningstid: 20 sekunder per meddelande
// - Viktad slumpmässig urval (betalningsförhållande)
// - ALDRIG samma kund två gånger i rad
// - Olika visningslägen: text, rullning, blinkning
// ============================================

#define DISPLAY_DURATION_MS 20000  // 20 sekunder
#define SCROLL_DELAY_MS 400        // Rullningshastighet
#define BLINK_DELAY_MS 500         // Blinkningshastighet

// Kund-ID:n
typedef enum {
    HARRY = 0,      // Hederlige Harrys Bilar - 5000 kr
    FARMOR_ANKA,    // Farmor Ankas Pajer AB - 3000 kr
    PETTER,         // Svarte Petters Svartbyggen - 1500 kr
    LANGBEN,        // Långbens detektivbyrå - 4000 kr
    IOT_REKLAM      // IOT:s Reklambyrå - 1000 kr
} Customer;

// Visningslägen
typedef enum {
    MODE_TEXT,
    MODE_SCROLL,
    MODE_BLINK
} DisplayMode;

// Meddelandestruktur
typedef struct {
    const char* text;
    DisplayMode mode;
    Customer customer;
} Message;

// Alla reklammeddelanden
Message messages[] = {
    // Hederlige Harrys Bilar (5000 kr) - 3 meddelanden, slumpmässigt
    {"Köp bil hos Harry", MODE_SCROLL, HARRY},
    {"God bilaffär Harry", MODE_TEXT, HARRY},
    {"Hederlige Harrys", MODE_BLINK, HARRY},
    
    // Farmor Ankas Pajer AB (3000 kr) - 2 meddelanden, slumpmässigt
    {"Köp paj hos Anka", MODE_SCROLL, FARMOR_ANKA},
    {"Skynda! Mårten äter", MODE_TEXT, FARMOR_ANKA},
    
    // Svarte Petters Svartbyggen (1500 kr) - 2 meddelanden, tidsbaserat
    {"Låt Petter bygga", MODE_SCROLL, PETTER},       // Jämna minuter
    {"Bygga svart? Ring", MODE_TEXT, PETTER},        // Udda minuter
    
    // Långbens detektivbyrå (4000 kr) - 2 meddelanden, slumpmässigt
    {"Mysterier? Långben", MODE_TEXT, LANGBEN},
    {"Långben fixar det", MODE_TEXT, LANGBEN},
    
    // IOT:s Reklambyrå (1000 kr) - 1 meddelande
    {"Synas här? IOTs", MODE_TEXT, IOT_REKLAM}
};

// Betalningsbelopp (bestämmer vikt i slumpmässigt urval)
const int customerPayments[] = {
    5000,  // HARRY
    3000,  // FARMOR_ANKA
    1500,  // PETTER
    4000,  // LANGBEN
    1000   // IOT_REKLAM
};

const int NUM_MESSAGES = sizeof(messages) / sizeof(messages[0]);
const int NUM_CUSTOMERS = sizeof(customerPayments) / sizeof(customerPayments[0]);

// Globala variabler
Customer lastCustomer = IOT_REKLAM;  // Initiera för att undvika första kollisionen
uint8_t minuteCounter = 0;           // För Petters tidsbaserade meddelanden

// Timer för millis()
volatile uint32_t milliseconds = 0;

// Initiera timer för millis() funktion (CTC 1ms)
void initTimer() {
    TCCR0A = (1 << WGM01);                                 // CTC-läge
    TCCR0B = (1 << CS01) | (1 << CS00);                    // prescaler 64
    OCR0A = 249;                                           // jämför för ~1 ms (16MHz/64/1000-1)
    TIMSK0 = (1 << OCIE0A);                                // aktivera compare match A avbrott
    sei();                                                 // aktivera globala avbrott
}

// ISR -> ökar globala millisekunder
ISR(TIMER0_COMPA_vect) {
    milliseconds++;
}

// Hämta millisekunder sedan start
uint32_t millis() {
    uint32_t ms;
    cli();
    ms = milliseconds;
    sei();
    return ms;
}


// ============================================
// KUNDURVAL
// ============================================

// Välj slumpmässig kund baserat på betalningsvikt
// Väljer ALDRIG samma kund två gånger i rad
Customer selectRandomCustomer() {
    // Beräkna total vikt exklusive senaste kunden
    int totalWeight = 0;
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        if ((Customer)i != lastCustomer) {
            totalWeight += customerPayments[i];
        }
    }
    
    // Generera slumpvärde
    int randomValue = rand() % totalWeight;
    int cumulativeWeight = 0;
    
    // Hitta kund baserat på viktad sannolikhet
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        if ((Customer)i != lastCustomer) {
            cumulativeWeight += customerPayments[i];
            if (randomValue < cumulativeWeight) {
                return (Customer)i;
            }
        }
    }
    
    return HARRY;  // Reservlösning (ska aldrig hända)
}

// Hämta meddelandeindex för en specifik kund
int getMessageForCustomer(Customer customer) {
    // Hitta alla meddelanden för denna kund
    int messageIndices[10];
    int count = 0;
    
    for (int i = 0; i < NUM_MESSAGES; i++) {
        if (messages[i].customer == customer) {
            messageIndices[count++] = i;
        }
    }
    
    // Specialfall: Petters tidsbaserade urval
    if (customer == PETTER) {
        // Jämn minut (0,2,4...): rullningsmeddelande
        // Udda minut (1,3,5...): textmeddelande
        return messageIndices[minuteCounter % 2];
    }
    
    // Slumpmässigt urval för andra kunder
    if (count > 0) {
        return messageIndices[rand() % count];
    }
    
    return 0;  // Reservlösning
}

// Hjälpfunktion för UTF-8 svenska tecken (för displayText och displayScroll)
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
// VISNINGSFUNKTIONER
// ============================================

// Visa statisk text (UTF-8-medveten)
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

// Visa rullande text (vänster rullning) - UTF-8 medveten
void displayScroll(HD44780& lcd, const char* text, uint32_t duration) {
    uint32_t startTime = millis();

    // Bygg array av glyf-pekare
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

// Visa blinkande text - NU använder UTF-8 medveten WriteText!
void displayBlink(HD44780& lcd, const char* text, uint32_t duration) {
    uint32_t startTime = millis();
    bool visible = true;
    
    while (millis() - startTime < duration) {
        if (visible) {
            lcd.Clear();
            lcd.GoTo(0, 0);
            lcd.WriteText((char*)text);  // WriteText är nu UTF-8 medveten!
        } else {
            lcd.Clear();
        }
        
        visible = !visible;
        _delay_ms(BLINK_DELAY_MS);
    }
}

// Visa ett meddelande baserat på dess läge
void displayMessage(HD44780& lcd, const Message& msg) {
    printf("Visar: %s (läge: %d, kund: %d)\n", 
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
// HUVUDPROGRAM
// ============================================

int main(void) {
    // Initiera seriell kommunikation
    init_serial();
    
    // Initiera LCD (anpassade tecken skapas automatiskt i Initialize())
    HD44780 lcd;
    lcd.Initialize();
    lcd.Clear();
    
    // Initiera timer för millis()
    initTimer();
    
    // Seeda slumptalsgenerator
    srand(TCNT0);
    
    printf("=================================\n");
    printf("REKLAMTAVLAN - Advertising Board\n");
    printf("=================================\n");
    printf("Kunder: 5\n");
    printf("Visningstid: 20 sekunder\n");
    printf("Systemet startat!\n\n");
    
    // Välkomstmeddelande
    displayText(lcd, "IOT Reklambyrå");
    _delay_ms(3000);
    lcd.Clear();
    displayText(lcd, "Reklamtavlan");
    _delay_ms(2000);
    
    // Huvudloop
    while(1) {
        // Välj nästa kund (viktad slumpmässig, aldrig samma två gånger)
        Customer nextCustomer = selectRandomCustomer();
        
        // Hämta meddelande för vald kund
        int messageIndex = getMessageForCustomer(nextCustomer);
        Message currentMessage = messages[messageIndex];
        
        // Debug-utdata
        printf("\n--- Nytt meddelande ---\n");
        printf("Kund: ");
        switch(nextCustomer) {
            case HARRY: printf("Hederlige Harrys Bilar\n"); break;
            case FARMOR_ANKA: printf("Farmor Ankas Pajer AB\n"); break;
            case PETTER: printf("Svarte Petters Svartbyggen\n"); break;
            case LANGBEN: printf("Långbens detektivbyrå\n"); break;
            case IOT_REKLAM: printf("IOT:s Reklambyrå\n"); break;
        }
        printf("Betalning: %d kr\n", customerPayments[nextCustomer]);
        
        // Visa meddelandet
        displayMessage(lcd, currentMessage);
        
        // Uppdatera tillstånd
        lastCustomer = nextCustomer;
        minuteCounter++;
        
        printf("Visning klar.\n");
    }
    
    return 0;
}