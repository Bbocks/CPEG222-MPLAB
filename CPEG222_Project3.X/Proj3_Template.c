/*===================================CPEG222====================================
 * Program:      Project 3 template
 * Authors:     Robert Freeman
 * Date:        10/12/2024
 * This is a template that you can use to write your project 3 code, for mid-stage and final demo.
==============================================================================*/
/*-------------- Board system settings. PLEASE DO NOT MODIFY THIS PART ----------*/
#ifndef _SUPPRESS_PLIB_WARNING          //suppress the plib warning during compiling
#define _SUPPRESS_PLIB_WARNING
#endif
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20         // PLL Multiplier (20x Multiplier)
#pragma config FPLLODIV = DIV_1         // System PLL Output Clock Divider (PLL Divide by 1)
#pragma config FNOSC = PRIPLL           // Oscillator Selection Bits (Primary Osc w/PLL (XT+,HS+,EC+PLL))
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config POSCMOD = XT             // Primary Oscillator Configuration (XT osc mode)
#pragma config FPBDIV = DIV_8           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/8)
/*----------------------------------------------------------------------------*/

#include <xc.h>   //Microchip XC processor header which links to the PIC32MX370512L header
#include <stdio.h>  // need this for sprintf
#include <sys/attribs.h>
#include "config.h" // Basys MX3 configuration header
#include "lcd.h"    // Digilent Library for using the on-board LCD
#include "acl.h"    // Digilent Library for using the on-board accelerometer
#include "ssd.h"

#define TRUE 1
#define FALSE 0

// below are keypad row and column definitions based on the assumption that JB will be used and columns are CN pins
// If you want to use JA or use rows as CN pins, modify this part
#define R4 LATCbits.LATC14
#define R3 LATDbits.LATD0
#define R2 LATDbits.LATD1
#define R1 LATCbits.LATC13
#define C4 PORTDbits.RD9
#define C3 PORTDbits.RD11
#define C2 PORTDbits.RD10
#define C1 PORTDbits.RD8

#define SYS_FREQ (80000000L) // 80MHz system clock
#define _80Mhz_ (80000000L)
#define LOOPS_NEEDED_TO_DELAY_ONE_MS_AT_80MHz 1426
#define LOOPS_NEEDED_TO_DELAY_ONE_MS (LOOPS_NEEDED_TO_DELAY_ONE_MS_AT_80MHz * (SYS_FREQ / _80Mhz_))
#define TMR_FREQ_SINE   48000 // 48 kHz

#define PB_FRQ (SYS_FREQ / 8)  // Peripheral clock is SYS_FREQ divided by 8
#define TMR4_PERIOD ((PB_FRQ / 256) / 1)  // Timer4 period for 1Hz, with 256 prescaler

unsigned short rgSinSamples [] = {
256,320,379,431,472,499,511,507,488,453,
406,350,288,224,162,106, 59, 24,  5,  1,
 13, 40, 81,133,192};

#define RGSIN_SIZE  (sizeof(rgSinSamples) / sizeof(rgSinSamples[0]))
unsigned short *pAudioSamples;

int cntAudioBuf, idxAudioBuf;

typedef enum _KEY {K0, K1, K2, K3, K4, K5, K6, K7, K8, K9, K_A, K_B, K_C, K_D, K_E, K_F, K_NONE} eKey ;
typedef enum _MODE {MODE1,MODE2} eModes ;

eModes mode = MODE1;

char new_press = FALSE;
eKey key = K_NONE;

// subroutines
void CNConfig();
void handle_new_keypad_press(eKey key) ;
void mode1();
void mode2();
void mode1_input(eKey key);
void mode2_input(eKey key);
void turnOnAlarm();
void turnOffAlarm();
void display_num();

// Variables to track LED status
int led_count = 8;  // Start with 8 LEDs on

void setupTimer4(void);
void setupLEDs(void);

int digit_count = 0;
int vals[3] = {0,0,0,0};
int count = 3;

int main(void) {

    /* Initialization of LED, LCD, SSD, etc */
    DDPCONbits.JTAGEN = 0; // Required to use Pin RA0 (connected to LED 0) as IO
    LCD_Init() ;
    ACL_Init();
    SSD_Init();

    float rgACLGVals[3];
    ACL_ReadGValues(rgACLGVals);
    int seed = rgACLGVals[0] * 10000;
    srand((unsigned) seed);
    // below are keypad row and column configurations based on the assumption that JB will be used and columns are CN pins
    // If you want to use JA or use rows as CN pins, modify this part

    // keypad rows as outputs
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;
    ANSELDbits.ANSD1 = 0;
    TRISCbits.TRISC14 = 0;
    TRISCbits.TRISC13 = 0;

    // keypad columns as inputs
    TRISDbits.TRISD8 = 1;
    TRISDbits.TRISD9 = 1;
    TRISDbits.TRISD10 = 1;
    TRISDbits.TRISD11 = 1;
    
    // You need to enable all the rows
    R1 = R2 = R3 = R4 = 0;
    
    LCD_WriteStringAtPos("   Project 3    ",0,0);
    LCD_WriteStringAtPos("    Mode 1!     ",1,0);
    
    CNConfig();
    setupLEDs();   // Set up LEDs for decrementing
    setupTimer4(); // Set up Timer4 for LED decrementing

    // the following lines configure interrupts to control the speaker
	T3CONbits.ON = 0;   	// turn off Timer3
	OC1CONbits.ON = 0;  	// Turn off OC1
    	/* The following code sets up the alarm timer and interrupts */
	tris_A_OUT = 0;    
	rp_A_OUT = 0x0C; // 1100 = OC1
    	// disable analog (set pins as digital)
	ansel_A_OUT = 0;
    
	T3CONbits.TCKPS = 0; 	//1:1 prescale value
	T3CONbits.TGATE = 0; 	//not gated input (the default)
	T3CONbits.TCS = 0;   	//PCBLK input (the default)
    
	OC1CONbits.ON = 0;   	// Turn off OC1 while doing setup.
	OC1CONbits.OCM = 6;  	// PWM mode on OC1; Fault pin is disabled
	OC1CONbits.OCTSEL = 1;   // Timer3 is the clock source for this Output Compare module
    
	IPC3bits.T3IP = 7;  	// interrupt priority
	IPC3bits.T3IS = 3;  	// interrupt subpriority
    
	macro_enable_interrupts();  // enable interrupts at CPU
    
    turnOffAlarm();
    
    SSD_WriteDigits(vals[0],vals[1],vals[2],vals[3],0,0,0,0);
    delay_ms(10);
    for (int i = 0; i < 4; i++) {
        vals[i] = -1;
    }
    SSD_WriteDigits(vals[0],vals[1],vals[2],vals[3],0,0,0,0);

    while (TRUE) 
    {
    }
} 


void CNConfig() {
    /* Make sure vector interrupts is disabled prior to configuration */
    macro_disable_interrupts;
    
    // Complete the following configuration of CN interrupts, then uncomment them
    CNCONDbits.ON = 1;   //all port D pins to trigger CN interrupts
    CNEND = 0xF00;      	//configure PORTD pins 8-11 as CN pins
    CNPUD = 0xF00;      	//enable pullups on PORTD pins 8-11

    IPC8bits.CNIP = 5;  	// set CN priority to  5
    IPC8bits.CNIS = 3;  	// set CN sub-priority to 3

    IFS1bits.CNDIF = 0;   	//Clear interrupt flag status bit
    IEC1bits.CNDIE = 1;   	//Enable CN interrupt on port D

    
    int j = PORTD;             //read port to clear mismatch on CN pins
    macro_enable_interrupts();	// re-enable interrupts
}

// This ISR is for the change notice interrupt. The timer will use another interrupt.
// Try looking at ssd.c for reference when setting it up! You will also source code from Project 2's alarm
// and put it on another timer for this project.

void __ISR(_CHANGE_NOTICE_VECTOR) CN_Handler(void) {
    //eKey key = K_NONE;
    
    // 1. Disable CN interrupts
    IEC1bits.CNDIE = 0;     

    // 2. Debounce keys for 10ms
    for (int i=0; i<1426; i++) {}

    // 3. Handle "button locking" logic

    unsigned char key_is_pressed = (!C1 || !C2 || !C3 || !C4);
    // If a key is already pressed, don't execute the rest second time to eliminate double pressing
    if (!key_is_pressed)
    {
        new_press = FALSE;
    }
    else if (!new_press)
    {
        new_press = TRUE;

        // 4. Decode which key was pressed
        
        // check first row 
        R1 = 0; R2 = R3 = R4 = 1;
        if (C1 == 0) { key = K1; }      // first column
        else if (C2 == 0) { key = K2; } // second column
        else if (C3 == 0) { key = K3; } // third column
        else if (C4 == 0) { key = K_A; } // fourth column

        // check second row 
        R2 = 0; R1 = R3 = R4 = 1;
        if (C1 == 0) { key = K4; }
        else if (C2 == 0) { key = K5; }
        else if (C3 == 0) { key = K6; }
        else if (C4 == 0) { key = K_B; }

        // check third row 
        R3 = 0; R1 = R2 = R4 = 1;
        if (C1 == 0) { key = K7; }
        else if (C2 == 0) { key = K8; }
        else if (C3 == 0) { key = K9; }
        else if (C4 == 0) { key = K_C; }

        // check fourth row 
        R4 = 0; R1 = R2 = R3 = 1;
        if (C1 == 0) { key = K0; }
        else if (C2 == 0) { key = K_F; }
        else if (C3 == 0) { key = K_E; }
        else if (C4 == 0) { key = K_D; }

        // re-enable all the rows for the next round
        R1 = R2 = R3 = R4 = 0;
        display_num();
        SSD_WriteDigits(vals[0],vals[1],vals[2],vals[3],0,0,0,0);
    
    }
    
    // if any key has been pressed, update next state and outputs
    if (key != K_NONE) {
        handle_new_keypad_press(key) ;
    }
    
    
    int j = PORTD;              //read port to clear mismatch on CN pints
    
    // 5. Clear the interrupt flag
    IFS1bits.CNDIF = 0;     

    // 6. Reenable CN interrupts
    IEC1bits.CNDIE = 1; 
}



void handle_new_keypad_press(eKey key)
{
    switch (mode)
    {
    case MODE1:
        mode1_input(key);
    break;
    case MODE2:
        mode2_input(key);
    break;
    }
}

void mode1(){
    mode = MODE1;

    LCD_WriteStringAtPos("    Mode 1!     ",1,0);
}

void mode2(){
    mode = MODE2;

    LCD_WriteStringAtPos("    Mode 2!     ",1,0);
}

void mode1_input(eKey key){
    //Go to mode 2 if A key is pressed
    switch(key){
        case K_A:
            mode2();
        break;
    }
}

void mode2_input(eKey key){
    //Go to mode 1 if any number key is pressed

    switch(key){
        case K0: case K1: case K2: case K3: case K4: case K5: case K6: case K7: case K8: case K9:
            mode1();
        break;
    }
}

void delay_ms(int milliseconds)
{
	int i;
	for (i = 0; i < milliseconds * LOOPS_NEEDED_TO_DELAY_ONE_MS; i++)
	{}
}

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

	T3CONbits.ON = 1;    	//turn on Timer3
	OC1CONbits.ON = 1;   	// Start the OC1 module  
	IEC0bits.T3IE = 1;  	// enable Timer3 interrupt    
	IFS0bits.T3IF = 0;  	// clear Timer3 interrupt flag
}

void turnOffAlarm()
{
	T3CONbits.ON = 0;   	// turn off Timer3
	OC1CONbits.ON = 0;  	// Turn off OC1
}

void __ISR(_TIMER_3_VECTOR, IPL7AUTO) Timer3ISR(void)
{  
    // play sine
	// load sine value into OC register
	OC1RS = 4*pAudioSamples[(++idxAudioBuf) % cntAudioBuf];
    
	IFS0bits.T3IF = 0;  	// clear Timer3 interrupt flag
}

void display_num() {
    switch(key){
        case K0: 
            vals[count] = 0;
            count--;
            break;
        case K1:
            vals[count] = 1;
            count--;
            break;
        case K2:
            vals[count] = 2;
            count--;
            break;
        case K3:
            vals[count] = 3;
            count--;
            break;
        case K4: 
            vals[count] = 4;
            count--;
            break;
        case K5: 
            vals[count] = 5;
            count--;
            break;
        case K6: 
            vals[count] = 6;
            count--;
            break;
        case K7: 
            vals[count] = 7;
            count--;
            break;
        case K8: 
            vals[count] = 8;
            count--;
            break;
        case K9:
            vals[count] = 9;
            count--;
            break;
        //case K_A:
        //case K_B:
        case K_C:
            for (int j = 0; j < 4; j++) {
                vals[j] = -1;
            }
            count = 3;
            break;
        case K_D:
            count++;
            vals[count] = -1;
            break;
        case K_E:
            if ((vals[0] != -1) && (vals[1] != -1) && (vals[2] != -1) && (vals[3] != -1)) {
                turnOnAlarm();
                delay_ms(10);
                turnOffAlarm();
                turnOnAlarm();
                delay_ms(10);
                turnOffAlarm();
                turnOnAlarm();
                delay_ms(10);
                turnOffAlarm();
                for (int j = 0; j < 4; j++) {
                    vals[j] = -1;
                }
                count = 3;
            }
            break;
        //case K_F:    
    }
}

void setupTimer4(void) {
    // Configure Timer4 for 1-second interrupts
    T4CONbits.TCKPS = 7;  // Set prescaler to 256
    PR4 = TMR4_PERIOD;    // Set period register for 1 second
    TMR4 = 0;             // Reset timer count to 0

    // Setup Timer4 interrupt
    IFS0bits.T4IF = 0;    // Clear interrupt flag
    IEC0bits.T4IE = 1;    // Enable Timer4 interrupt
    IPC4bits.T4IP = 5;    // Set priority level to 5

    // Start Timer4
    T4CONbits.ON = 1;     // Turn on Timer4
}

void setupLEDs(void) {
    // Configure LEDs (assuming LD0-LD7 are connected to PORTA)
    TRISAbits.TRISA0 = 0; // Set RA0 (LD0) as output
    TRISAbits.TRISA1 = 0; // Set RA1 (LD1) as output
    TRISAbits.TRISA2 = 0; // Set RA2 (LD2) as output
    TRISAbits.TRISA3 = 0; // Set RA3 (LD3) as output
    TRISAbits.TRISA4 = 0; // Set RA4 (LD4) as output
    TRISAbits.TRISA5 = 0; // Set RA5 (LD5) as output
    TRISAbits.TRISA6 = 0; // Set RA6 (LD6) as output
    TRISAbits.TRISA7 = 0; // Set RA7 (LD7) as output

    // Initially turn on all LEDs
    LATA = 0xFF;
}

void __ISR(_TIMER_4_VECTOR, IPL5SOFT) Timer4ISR(void) {
    // Timer4 ISR triggered every 1 second
    if (led_count > 0) {
        led_count--;  // Decrement the number of LEDs on
    } else {
        led_count = 8;  // Reset to 8 LEDs after reaching 0
    }

    // Update LED states: turn on only the `led_count` most significant LEDs
    LATA = (0xFF >> (8 - led_count));  // Shift the bits to light the correct number of LEDs

    // Clear the interrupt flag
    IFS0bits.T4IF = 0;
}