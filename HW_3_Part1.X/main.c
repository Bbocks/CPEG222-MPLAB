#include <xc.h>
#include "config.h"
#include "lcd.h"

// Define TRUE and FALSE for logic
#define TRUE 1
#define FALSE 0

// Define constants for delay calculation
#define SYS_FREQ (80000000L) // 80MHz system clock
#define PB_DIV 8  // Peripheral Bus Clock divisor
#define PB_FREQ (SYS_FREQ / PB_DIV)  // Peripheral Bus Clock frequency
#define LOOPS_NEEDED_TO_DELAY_ONE_MS (PB_FREQ / 2000)  // Approximate loop count for 1 ms delay

// Define button and switch mappings
#define BUTTON_DEBOUNCE_DELAY_MS 20
#define BUTTON_RIGHT PORTBbits.RB8
#define SWITCH_SPEED PORTBbits.RB10 // SW6 now connected to RB10
#define SWITCH_DIRECTION PORTBbits.RB9 // SW7 now connected to RB9

// Mode enumeration
enum mode { MODE1, MODE2, MODE3, MODE4 };

char buttonsLocked = FALSE;
char pressedUnlockedBtnR = FALSE;

// Function prototypes
void initialize_ports();
void handle_button_presses();
void delay_ms(int milliseconds);
void logic_mode_one();
void logic_mode_two(enum mode *current_mode);
void logic_mode_three();
void logic_mode_four(enum mode *current_mode);
void logic_button_presses(enum mode *modePtr);
void update_leds(int pattern);

// Main Function
int main(void)
{
    initialize_ports();
    enum mode current_mode = MODE1;

    while (1)
    {
        handle_button_presses();
        if (pressedUnlockedBtnR) // Actions when BTNR is pressed
        {
            logic_button_presses(&current_mode);
        }

        switch (current_mode)
        {
            case MODE1:
                logic_mode_one();
                break;
            case MODE2:
                logic_mode_two(&current_mode); // Pass current mode pointer
                break;
            case MODE3:
                logic_mode_three();
                break;
            case MODE4:
                logic_mode_four(&current_mode); // Pass current mode pointer
                break;
        }
    }
}

// Initialize ports
void initialize_ports()
{
    DDPCONbits.JTAGEN = 0;
    TRISA &= 0xFF00; // Set Port A as output for LEDs
    TRISBbits.TRISB8 = 1; // Set RB8 (BTNR) as input
    ANSELBbits.ANSB8 = 0; // Disable analog input on RB8
    TRISBbits.TRISB10 = 1; // Set RB10 (SW6) as input (digital)
    ANSELBbits.ANSB10 = 0; // Disable analog input on RB10
    TRISBbits.TRISB9 = 1;  // Set RB9 (SW7) as input (digital)
    ANSELBbits.ANSB9 = 0;  // Disable analog input on RB9
    LCD_Init();
    LCD_WriteStringAtPos("    Group #1    ", 0, 0);
}

// Handle button presses with debounce
void handle_button_presses()
{
    pressedUnlockedBtnR = FALSE;
    if (BUTTON_RIGHT && !buttonsLocked)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLocked = TRUE;
        pressedUnlockedBtnR = TRUE;
    }
    else if (!BUTTON_RIGHT && buttonsLocked)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLocked = FALSE;
    }
}

// Delays in milliseconds
void delay_ms(int milliseconds)
{
    int i;
    for (i = 0; i < milliseconds * LOOPS_NEEDED_TO_DELAY_ONE_MS; i++)
    {
        asm("nop"); // no-operation to waste time
    }
}

// Mode transition logic
void logic_button_presses(enum mode *modePtr)
{
    if (*modePtr == MODE1)
        *modePtr = MODE2;
    else if (*modePtr == MODE2)
        *modePtr = MODE3;
    else if (*modePtr == MODE3)
        *modePtr = MODE4;
    else
        *modePtr = MODE1;
}

// Mode 1: All LEDs ON
void logic_mode_one()
{
    LCD_WriteStringAtPos("    Group #1    ", 0, 0);
    LCD_WriteStringAtPos("     Mode 1     ", 1, 0);
    LATA |= 0x00FF; // Turn all LEDs ON
}

// Mode 2: Turn OFF LEDs in pairs or one by one depending on SW6
void logic_mode_two(enum mode *current_mode)
{
    LCD_WriteStringAtPos("    Group #1    ", 0, 0);
    LCD_WriteStringAtPos("     Mode 2     ", 1, 0);

    int pattern = 0xFF;  // Start with all LEDs ON
    int increment = (SWITCH_SPEED == 0) ? 1 : 2;  // 1 for single, 2 for pairs based on SW6

    if (SWITCH_DIRECTION == 0) // Right to left
    {
        // Turn off LEDs from right to left in increments of 1 or 2
        for (int i = 0; i < 8; i += increment)
        {
            pattern >>= increment;  // Shift pattern right by increment (1 or 2 bits)
            update_leds(pattern);
            delay_ms(SWITCH_SPEED ? 100 : 300);  // Fast (100ms) or slow (300ms) delay
        }
    }
    else // Left to right
    {
        // Turn off LEDs from left to right in increments of 1 or 2
        for (int i = 0; i < 8; i += increment)
        {
            pattern <<= increment;  // Shift pattern left by increment (1 or 2 bits)
            update_leds(pattern);
            delay_ms(SWITCH_SPEED ? 100 : 300);  // Fast or slow delay
        }
    }

    // Ensure all LEDs are off before moving to Mode 3
    update_leds(0x00);  // Turn off all LEDs
    *current_mode = MODE3;  // Transition to Mode 3
}

// Mode 3: All LEDs OFF
void logic_mode_three()
{
    LCD_WriteStringAtPos("    Group #1    ", 0, 0);
    LCD_WriteStringAtPos("     Mode 3     ", 1, 0);
    LATA &= 0xFF00; // Turn all LEDs OFF
}

// Mode 4: Turn ON LEDs in increments based on switches, similar to Mode 2 but turning LEDs ON
void logic_mode_four(enum mode *current_mode)
{
    LCD_WriteStringAtPos("    Group #1    ", 0, 0);
    LCD_WriteStringAtPos("     Mode 4     ", 1, 0);

    int pattern = 0x00;  // Start with all LEDs OFF
    int increment = (SWITCH_SPEED == 0) ? 1 : 2;  // Set increment based on SW6 (1 for single, 2 for pairs)

    if (SWITCH_DIRECTION == 0) // Right to left
    {
        // Turn on LEDs from right to left in increments of 1 or 2
        for (int i = 0; i < 8; i += increment)
        {
            // Shift pattern for right-to-left direction and add bits accordingly
            pattern |= (0x80 >> i) | (increment == 2 ? (0x80 >> (i + 1)) : 0);
            update_leds(pattern);
            delay_ms(SWITCH_SPEED ? 100 : 300);  // Adjust delay for speed (100ms for fast, 300ms for slow)
        }
    }
    else // Left to right
    {
        // Turn on LEDs from left to right in increments of 1 or 2
        for (int i = 0; i < 8; i += increment)
        {
            // Shift pattern for left-to-right direction and add bits accordingly
            pattern |= (0x01 << i) | (increment == 2 ? (0x01 << (i + 1)) : 0);
            update_leds(pattern);
            delay_ms(SWITCH_SPEED ? 100 : 300);  // Adjust delay for speed (100ms for fast, 300ms for slow)
        }
    }

    // Ensure all LEDs are on before moving to Mode 1
    update_leds(0xFF);  // Turn on all LEDs
    *current_mode = MODE1;  // Transition to Mode 1
}




// Update LEDs with a specific pattern
void update_leds(int pattern)
{
    LATA &= 0xFF00;  // Clear LEDs
    LATA |= (pattern & 0x00FF);  // Set pattern on LEDs
}
