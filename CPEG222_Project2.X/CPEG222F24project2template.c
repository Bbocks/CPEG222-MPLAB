/*===================================CPEG222====================================
 * Program: Proj2_Template.c
 * Authors:  Brett Bockstein and Cole Smith
 * Date:  10/2/2024
 * Description: This template uses an on-board BTN to control
 * the alarm, turning it on and off, and the SSD, which will display
 * the number of times btnC is pressed.
 * Output: All LEDs are turned on and off.
==============================================================================*/
/*---- Board system settings. PLEASE DO NOT MODIFY THIS PART FOR PROJECT 1 ---*/
#ifndef _SUPPRESS_PLIB_WARNING //suppress the plib warning during compiling
#define _SUPPRESS_PLIB_WARNING
#endif
#pragma config FPLLIDIV = DIV_2 // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20 // PLL Multiplier (20x Multiplier)
#pragma config FPLLODIV = DIV_1 // System PLL Output Clock Divider 
                                //(PLL Divide by 1)
#pragma config FNOSC = PRIPLL   // Oscillator Selection Bits (Primary Osc w/PLL 
                                //(XT+,HS+,EC+PLL))
#pragma config FSOSCEN = OFF    // Secondary Oscillator Enable (Disabled)
#pragma config POSCMOD = XT     // Primary Oscillator Configuration (XT osc mode)
#pragma config FPBDIV = DIV_8   // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/8)
/*----------------------------------------------------------------------------*/
#include <xc.h>     //Microchip XC processor header which links to the 
                    //PIC32MX370512L header
#include <sys/attribs.h>
#include "config.h" // Basys MX3 configuration header
#include "lcd.h"    // NOTE: utils.c and utils.h must also be in your project 
                    //to use lcd.c
#include"ssd.h"
/* --------------------------- Forward Declarations-------------------------- */
void initialize_ports();
void initialize_output_states();
void handle_button_presses();
void delay_ms(int milliseconds);
void turnOnAlarm() ;
void turnOffAlarm() ;
/* ------------------------ Constant Definitions ---------------------------- */
#define SYS_FREQ (80000000L) // 80MHz system clock
#define _80Mhz_ (80000000L)
#define LOOPS_NEEDED_TO_DELAY_ONE_MS_AT_80MHz 1426
#define LOOPS_NEEDED_TO_DELAY_ONE_MS (LOOPS_NEEDED_TO_DELAY_ONE_MS_AT_80MHz * (SYS_FREQ / _80Mhz_))
#define BtnC_RAW PORTFbits.RF0
#define BtnU_RAW PORTBbits.RB1
#define BtnD_RAW PORTAbits.RA15
#define BtnL_RAW PORTBbits.RB0
#define BtnR_RAW PORTBbits.RB8
#define TRUE 1
#define FALSE 0
#define BUTTON_DEBOUNCE_DELAY_MS 20

/***** This section contains variables for the speaker ******/
#define TMR_FREQ_SINE   48000 // 48 kHz
// This array contains the values that implement one syne period, over 25 samples. 
// They are generated using this site: 
// http://www.daycounter.com/Calculators/Sine-Generator-Calculator.phtml
unsigned short rgSinSamples [] = {
256,320,379,431,472,499,511,507,488,453,
406,350,288,224,162,106, 59, 24,  5,  1,
 13, 40, 81,133,192};

#define RGSIN_SIZE  (sizeof(rgSinSamples) / sizeof(rgSinSamples[0]))

// the array of samples, to be stored when recording (Mode 2) and to be played when playing back (Mode 3).
unsigned short *pAudioSamples;

// global variables that store audio buffer position and size
int cntAudioBuf, idxAudioBuf;
/***** End of speaker declararions  ******/  // shouldn't touch

typedef enum {ALARM_ON, ALARM_OFF} eModes ;
/* -------------------- Global Variable Declarations ------------------------ */
char buttonsLockedC = FALSE;
char pressedUnlockedBtnC = FALSE;
char buttonsLockedU = FALSE;
char pressedUnlockedBtnU = FALSE;
char buttonsLockedD = FALSE;
char pressedUnlockedBtnD = FALSE;
char buttonsLockedR = FALSE;
char pressedUnlockedBtnR = FALSE;
char buttonsLockedL = FALSE;
char pressedUnlockedBtnL = FALSE;
eModes mode = ALARM_OFF ; 
char val0 = 0;
char val1 = 0;
char val2 = 0;
char val3 = 0;
int state = 1; 
int side = 0;
/* ----------------------------- Main --------------------------------------- */
int main(void)
{
    /*-------------------- Port and State Initialization -------------------------*/
    initialize_ports();            // Initialize ports for input/output
    initialize_output_states();    // Set initial output states
    RGBLED_Init();                 // Initialize the RGB LED

    // Time and alarm arrays, initialized with modulo to restrict values to single digits
    char vals[3] = {val0 % 10, val1 % 10, val2 % 10, val3 % 10};
    char alrm[3] = {val0 % 10, val1 % 10, val2 % 10, val3 % 10};
    
    alrm[3] = 1;                   // Set initial alarm hour
    alrm[2] = 2;                   // Set initial alarm minute

    int count = 0;                 // Counter for alarm mode blinking

    while (TRUE)
    {
        vals[3] = 0;               // Reset time seconds digit
        delay_ms(500);             // Delay for timing

        /*-------------------- Main logic and actions start --------------------------*/
        handle_button_presses();   // Check for button presses

        // State 1: Set Time Mode
        while (state == 1) {
            LCD_WriteStringAtPos("    Group #37     ", 0, 0); // Display group information
            LCD_WriteStringAtPos("    Set Time      ", 1, 0); // Prompt user to set time

            handle_button_presses(); // Check for button input

            if (pressedUnlockedBtnC) { 
                state = 3; // Switch to Display Time Mode if Button C is pressed
            }

            if (pressedUnlockedBtnL) { 
                side = (side == 1) ? 0 : 1; // Toggle between hours and minutes
            }

            if (pressedUnlockedBtnR) { 
                vals[0] = vals[1] = vals[2] = vals[3] = 0; // Reset time to 00:00
            }

            if (pressedUnlockedBtnU) {
                // Increment time based on selected side
                if (side == 0) { 
                    if (vals[1] == 5 && vals[0] == 9) {
                        vals[0] = vals[1] = 0;
                    } else if (vals[0] == 9) {
                        vals[0] = 0;
                        vals[1]++;
                    } else {
                        vals[0]++;
                    }
                } else if (side == 1) { 
                    if (vals[3] == 2 && vals[2] == 3) {
                        vals[2] = vals[3] = 0;
                    } else if (vals[2] == 9) {
                        vals[2] = 0;
                        vals[3]++;
                    } else {
                        vals[2]++;
                    }
                }
            }

            if (pressedUnlockedBtnD) {
                // Decrement time based on selected side
                if (side == 0) {
                    if (vals[1] == 0 && vals[0] == 0) {
                        vals[0] = 9;
                        vals[1] = 5;
                    } else if (vals[0] == 0) {
                        vals[0] = 9;
                        vals[1]--;
                    } else {
                        vals[0]--;
                    }
                } else if (side == 1) {
                    if (vals[3] == 0 && vals[2] == 0) {
                        vals[2] = 3;
                        vals[3] = 2;
                    } else if (vals[2] == 0 && vals[3] > 0) {
                        vals[2] = 9;
                        vals[3]--;
                    } else {
                        vals[2]--;
                    }
                }
            }

            SSD_WriteDigits(vals[0], vals[1], vals[2], vals[3], 0, 0, 0, 0); // Display current time
            delay_ms(10); // Short delay for display update
        }

        // State 2: Set Alarm Mode
        while (state == 2) {
            LCD_WriteStringAtPos("    Group #37     ", 0, 0); // Display group information
            LCD_WriteStringAtPos("    Set Alarm     ", 1, 0); // Prompt user to set alarm

            handle_button_presses(); // Check for button input

            // Increment alarm values for demonstration
            vals[0]++;
            if (vals[0] == 10) {
                vals[0] = 0;
                vals[1]++;
            }
            if (vals[1] == 6 && vals[0] == 0) {
                vals[1] = vals[0] = 0;
                vals[2]++;
            }
            if (vals[2] == 9) {
                vals[2] = 0;
                vals[3]++;
            }

            // Process button inputs and control alarm setting loop
            for (int j = 0; j <= 400; j++) {
                handle_button_presses();
                delay_ms(1);

                if (pressedUnlockedBtnC) state = 3; // Move to Display Time Mode
                if (pressedUnlockedBtnL) side = (side == 1) ? 0 : 1; // Toggle between hours and minutes
                if (pressedUnlockedBtnR) { 
                    vals[0] = alrm[0];
                    vals[1] = alrm[1];
                    vals[2] = alrm[2];
                    vals[3] = alrm[3];
                    state = 1; // Restore time and switch to Set Time Mode
                }
            }

            SSD_WriteDigits(alrm[0], alrm[1], alrm[2], alrm[3], 0, 0, 0, 0); // Display alarm time
            delay_ms(10); // Short delay for display update
        }

        // State 3: Display Time Mode
        while (state == 3) {
            LCD_WriteStringAtPos("    Group #37     ", 0, 0); // Display group information
            LCD_WriteStringAtPos("   Display Time   ", 1, 0); // Prompt user for time display

            handle_button_presses(); // Check for button input

            // Increment time for demonstration purposes
            vals[0]++;
            if (vals[0] == 10) {
                vals[0] = 0;
                vals[1]++;
            }
            if (vals[1] == 6 && vals[0] == 0) {
                vals[1] = vals[0] = 0;
                vals[2]++;
            }
            if (vals[2] == 9) {
                vals[2] = 0;
                vals[3]++;
            }

            // Handle inputs within display time loop
            for (int i = 0; i <= 400; i++) {
                handle_button_presses();
                delay_ms(1);
                if (pressedUnlockedBtnR) { 
                    side = 0;
                    state = 1; // Go back to Set Time Mode
                    vals[0] = vals[1] = vals[2] = vals[3] = 0;
                } else if (pressedUnlockedBtnC) {
                    state = 2; // Switch to Set Alarm Mode
                }
            }

            // If current time matches alarm time, switch to Alarm Mode
            if ((vals[0] == alrm[0]) && (vals[1] == alrm[1]) && (vals[2] == alrm[2]) && (vals[3] == alrm[3])) {
                state = 4;
            }

            SSD_WriteDigits(vals[0], vals[1], vals[2], vals[3], 0, 0, 0, 0); // Display current time
            delay_ms(10); // Short delay for display update
        }

        // State 4: Alarm Mode
        while (state == 4) {
            LCD_WriteStringAtPos("    Group #37     ", 0, 0); // Display group information
            LCD_WriteStringAtPos("    Alarming      ", 1, 0); // Indicate alarm is active

            handle_button_presses(); // Check for button input

            // Alarm blinking logic
            if ((count++ % 2) == 0) {
                SSD_WriteDigits(vals[0], vals[1], vals[2], vals[3], 0, 0, 0, 0); // Display current time
                RGBLED_SetValue(255, 0, 0); // Set RGB LED to red
                mode = ALARM_ON;            // Set alarm mode to ON
                turnOnAlarm();
            } else {
                SSD_WriteDigits(-1, -1, -1, -1, 0, 0, 0, 0); // Blink display off
                RGBLED_SetValue(0, 0, 0);    // Turn off RGB LED
                mode = ALARM_OFF;            // Set alarm mode to OFF
                turnOffAlarm();
            }

            // Check if alarm should be turned off or return to time display
            if (count == 10 || (pressedUnlockedBtnC && pressedUnlockedBtnR)) {
                RGBLED_SetValue(0, 0, 0);    // Turn off RGB LED
                mode = ALARM_OFF;            // Set alarm mode to OFF
                turnOffAlarm();
                state = 3;                   // Return to Display Time Mode
            }
            delay_ms(10); // Short delay for alarm update
        }
    }
    RGBLED_Close(); // Cleanup and close RGB LED
}

void initialize_ports()
{
    DDPCONbits.JTAGEN = 0; // Required to use Pin RA0 (connected to LED 0) as IO

    /* The following line sets the tristate of Port F bit 0 to 1.
     *  BtnC is connected to that pins. When the tristate of a pin is set high,
     *  the pin is configured as a digital input. */
    TRISFbits.TRISF0 = 1;
    TRISBbits.TRISB1 = 1; // RB1 (BTNU) configured as input
    ANSELBbits.ANSB1 = 0; // RB1 (BTNU) disabled analog
    TRISBbits.TRISB0 = 1; // RB1 (BTNL) configured as input
    ANSELBbits.ANSB0 = 0; // RB1 (BTNL) disabled analog
    TRISBbits.TRISB8 = 1; // RB8 (BTNR) configured as input
    ANSELBbits.ANSB8 = 0; // RB8 (BTNR) disabled analog
    TRISAbits.TRISA15 = 1; // RA15 (BTND) configured as input

    
    LCD_Init(); // A library function provided by Digilent
    SSD_Init();    //SSD Init

    // the following lines configure interrupts to control the speaker
    T3CONbits.ON = 0;       // turn off Timer3
    OC1CONbits.ON = 0;      // Turn off OC1
        /* The following code sets up the alarm timer and interrupts */
    tris_A_OUT = 0;    
    rp_A_OUT = 0x0C; // 1100 = OC1
        // disable analog (set pins as digital)
    ansel_A_OUT = 0;
    
    T3CONbits.TCKPS = 0;     //1:1 prescale value
    T3CONbits.TGATE = 0;     //not gated input (the default)
    T3CONbits.TCS = 0;       //PCBLK input (the default)
    
    OC1CONbits.ON = 0;       // Turn off OC1 while doing setup.
    OC1CONbits.OCM = 6;      // PWM mode on OC1; Fault pin is disabled
    OC1CONbits.OCTSEL = 1;   // Timer3 is the clock source for this Output Compare module
    
    IPC3bits.T3IP = 7;      // interrupt priority
    IPC3bits.T3IS = 3;      // interrupt subpriority
    
    macro_enable_interrupts();  // enable interrupts at CPU

}
void initialize_output_states()
{

    LCD_WriteStringAtPos("    Group #37     ", 0, 0); //Display "Welcome" at line 0,
    LCD_WriteStringAtPos("    Set Time   ", 1, 0); //Display "Press BtnC" at line 
 
    turnOffAlarm();
}
/* This below function turns on the alarm*/
void turnOnAlarm()
{
    //set up alarm
    PR3 = (int)((float)((float)PB_FRQ/TMR_FREQ_SINE) + 0.5);               
    idxAudioBuf = 0;
    cntAudioBuf = RGSIN_SIZE;
    pAudioSamples = rgSinSamples;

    // load first value
    OC1RS = pAudioSamples[0];
    TMR3 = 0;

    T3CONbits.ON = 1;        //turn on Timer3
    OC1CONbits.ON = 1;       // Start the OC1 module  
    IEC0bits.T3IE = 1;      // enable Timer3 interrupt    
    IFS0bits.T3IF = 0;      // clear Timer3 interrupt flag
}
/* This below function turns off the alarm*/
void turnOffAlarm()
{
    T3CONbits.ON = 0;       // turn off Timer3
    OC1CONbits.ON = 0;      // Turn off OC1
}
/* The below function only handles BtnC presses. Think about how it could be
 copied to handle all button presses.*/ 
void handle_button_presses()
{
    pressedUnlockedBtnC = FALSE;
    if (BtnC_RAW && !buttonsLockedC)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLockedC = TRUE;
        pressedUnlockedBtnC = TRUE;
    }
    else if (!BtnC_RAW && buttonsLockedC)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLockedC = FALSE;
    }
    
    pressedUnlockedBtnU = FALSE;
    if (BtnU_RAW && !buttonsLockedU)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLockedU = TRUE;
        pressedUnlockedBtnU = TRUE;
    }
    else if (!BtnU_RAW && buttonsLockedU)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLockedU = FALSE;
    }
    
    pressedUnlockedBtnL = FALSE;
    if (BtnL_RAW && !buttonsLockedL)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLockedL = TRUE;
        pressedUnlockedBtnL = TRUE;
    }
    else if (!BtnL_RAW && buttonsLockedL)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLockedL = FALSE;
    }
    
    pressedUnlockedBtnR = FALSE;
    if (BtnR_RAW && !buttonsLockedR)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLockedR = TRUE;
        pressedUnlockedBtnR = TRUE;
    }
    else if (!BtnR_RAW && buttonsLockedR)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLockedR = FALSE;
    }
    
    pressedUnlockedBtnD = FALSE;
    if (BtnD_RAW && !buttonsLockedD)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLockedD = TRUE;
        pressedUnlockedBtnD = TRUE;
    }
    else if (!BtnD_RAW && buttonsLockedD)
    {
        delay_ms(BUTTON_DEBOUNCE_DELAY_MS); // debounce
        buttonsLockedD = FALSE;
    }
}
void delay_ms(int milliseconds)
{
    int i;
    for (i = 0; i < milliseconds * LOOPS_NEEDED_TO_DELAY_ONE_MS; i++) 
    {}
}

/* ------------------------------------------------------------ */
/***	Timer3ISR
**
**	Description:
**		This is the interrupt handler for Timer3. According to each mode, it is called at specific frequencies, as initialized in AUDIO_Init.
    Mode 0 (Generate sound using sine) - Advance current index in the sine definition buffer, initialize OC1 with the current sine definition value.         
*/
void __ISR(_TIMER_3_VECTOR, IPL7AUTO) Timer3ISR(void) 
{  
   // play sine
    // load sine value into OC register
    OC1RS = 4*pAudioSamples[(++idxAudioBuf) % cntAudioBuf];
    
    IFS0bits.T3IF = 0;      // clear Timer3 interrupt flag
}

/**In this template no RGB code is provided. But the steps to use it are the same as the steps to use 
 * another aspect of the board which we do provide. Check rgb.c and rgb.h as well as the functions we give you
 * to see what you need to add to use the RGB.
**/
