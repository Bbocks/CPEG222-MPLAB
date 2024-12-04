/*===================================CPEG222====================================
 * Program:      Project 4 Template
 * Authors:     Brett Bockstein and Cole Smith
 * Date:        11/07/2024
 * This is a guide that you can use to write your project 4 code
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
#define SYS_FREQ (80000000L) // 80MHz system clock
#define _80Mhz_ (80000000L)
#define LOOPS_NEEDED_TO_DELAY_ONE_MS_AT_80MHz 1426
#define LOOPS_NEEDED_TO_DELAY_ONE_MS (LOOPS_NEEDED_TO_DELAY_ONE_MS_AT_80MHz * (SYS_FREQ / _80Mhz_))

#define TRUE 1
#define FALSE 0

#define TMR_TIME    0.020 // 3000 us for each tick

// Libraries
#include <string.h>
#include <xc.h>   //Microchip XC processor header which links to the PIC32MX370512L header
#include <stdio.h>  // need this for sprintf
#include <sys/attribs.h>
#include "config.h" // Basys MX3 configuration header
#include "led.h"
#include "ssd.h"
#include "lcd.h"
#include "swt.h"
#include "utils.h"

#define sw0 PORTFbits.RF3
#define sw1 PORTFbits.RF5
#define sw6 PORTBbits.RB10
#define sw7 PORTBbits.RB9

#define LD0 PORTAbits.RA0
#define LD1 PORTAbits.RA1
#define LD2 PORTAbits.RA2
#define LD3 PORTAbits.RA3
#define LD4 PORTAbits.RA4
#define LD5 PORTAbits.RA5
#define LD6 PORTAbits.RA6
#define LD7 PORTAbits.RA7

#define PM1 PORTCbits.RC3
#define PM2 PORTGbits.RG7
#define PM3 PORTGbits.RG8
#define PM4 PORTGbits.RG9

#define TRUE 1
#define FALSE 0
#define BUTTON_DEBOUNCE_DELAY_MS 20
#define BUTTON_RIGHT PORTBbits.RB8
char buttonsLocked = FALSE;
char pressedUnlockedBtnR = FALSE;

// Function Declarations
void initializePorts();
void pwmConfig();
void activateServo();
void Timer2Setup();
void Timer3Setup();
void setupLEDs(void);
void MIC_Init();

int vals[] = {-1,-1,-1,-1};
int led_vals;
char left[3];
char right[3];
int start;
int pmCount;
int tmrStart;


int main(void) {
    intializePorts();
    pwmConfig();
    Timer2Setup();
    Timer3Setup();
    setupLEDs();
    MIC_Init();
    for (int i = 0; i < 0; i ++) {
        LED_SetValue(i,0);
    }
    
    LCD_WriteStringAtPos("Team: Mr. Krabs",0,0);
    LCD_WriteStringAtPos("STP          STP",1,0);
    
    start = 0;
    
    while (TRUE) {
        //When a clap is heard, increment the start value
        if (MIC_Val() > 1000) {
            start++;
        }
        //If switch 7 is flipped to high, set start = 2
        if (sw7) {
            start = 2;
        } 
        //If start = 2, start the movement of the bot
        if (start == 2) {
            OC4RS = PR3 / 10; //Forwards
            sprintf(left, "FWD");
            OC5RS = PR3 / 20; //Forward
            sprintf(right, "FWD");
            char array[16];
            sprintf(array, "%s          %s", left, right);
            LCD_WriteStringAtPos(array, 1, 0);
            start++;
            tmrStart = 1;
        }
    }
}




// Initialize ports on board
void intializePorts() {
    DDPCONbits.JTAGEN = 0; // Required to use Pin RA0 (connected to LED 0) as IO
    LED_Init();
    LCD_Init();
    SSD_Init();
    SWT_Init();
    
    TRISBbits.TRISB9 = 1; //RB9 (SW7) configured as input
    ANSELBbits.ANSB9 = 0; //RB9 (SW7) disable analog
    
    TRISCbits.TRISC3 = 1; //JA7
    TRISGbits.TRISG7 = 1; //JA8
    TRISGbits.TRISG8 = 1; //JA9
    TRISGbits.TRISG9 = 1; //JA10
    ANSELGbits.ANSG7 = 0; //Disable analog JA8
    ANSELGbits.ANSG8 = 0; //Disable analog JA9
    ANSELGbits.ANSG9 = 0; //Disable analog JA10
    
}

void pwmConfig() {
    
    OC4CONbits.ON = 0;
    OC5CONbits.ON = 0;
    
    // Configure Output Compare Module 4
    
    OC4CONbits.OCM = 6;      // PWM mode on OC4; Fault pin is disabled
    OC4CONbits.OCTSEL = 1;   // Select the timer to use as a clock source
    OC4RS = PR3/20;//OC4RS is some fraction of the Period
    OC4R = OC4RS;
    OC4CONbits.ON = 1;       // Start the OC4 module
    
    //Do The same for OC5**************************
    OC5CONbits.OCM = 6;      // PWM mode on OC5; Fault pin is disabled
    OC5CONbits.OCTSEL = 1;   // Select the timer to use as a clock source
    OC5RS = PR3/20;//OC5RS is some fraction of the Period
    OC5R = OC5RS;
    OC5CONbits.ON = 1;       // Start the OC5 module
   
   
   TRISBbits.TRISB8 = 0; //set servo 0 as output
   TRISAbits.TRISA15 = 0; //set servo 1 as output
   ANSELBbits.ANSB8 = 0; //set servo 0 as digital

   RPB8R = 0x0B; // connect Servo 0 to OC5
   RPA15R = 0x0B;// connect Servo 1 to OC4
    

    //Set up additional timers here if necessary
}


//ISR's are here is you need them. Don't forget to set them up!
void __ISR(_TIMER_2_VECTOR) Timer2ISR(void) {
    IEC0bits.T2IE = 0; // disable interrupt
    
    if (tmrStart == 1){
        vals[0]++;
        if (vals[0]%10==0) {
            vals[0] = 0;
            vals[1]++;
        } 
        if (vals[1] == 10 && vals[0]%10==0) {
            if (vals[2] == -1) {
                vals[2]++;
            }
            vals[1] = 0;
            vals[2]++;
        } 
        if (vals[2] == 10 && vals[1]%10==0) {
            if (vals[3] == -1) {
                vals[3]++;
            }
            vals[2] = 0;
            vals[3]++;
        } 
        if (vals[3] == 10) {
            vals[3] = 0;
        }
        SSD_WriteDigits(vals[0],vals[1],vals[2],vals[3],0,1,0,0);
    } else {
        SSD_WriteDigits(vals[0],vals[1],vals[2],vals[3],0,1,0,0);
        vals[0] = -1;
        vals[1] = -1;
        vals[2] = -1;
        vals[3] = -1;
    }
    
    IFS0bits.T2IF = 0; // clear interrupt flag
    IEC0bits.T2IE = 1; // enable interrupt
}

void __ISR(_TIMER_3_VECTOR) Timer3ISR(void) {
    IEC0bits.T3IE = 0; // disable interrupt
    
    //If the middle two IR sensors see black, go forward
    if (PM1 && !PM2 && !PM3 && PM4) {
        OC4RS = PR3 / 13; //Forwards
        sprintf(left, "FWD");
        OC5RS = PR3 / 15; //Forward
        sprintf(right, "FWD");
        char array[16];
        sprintf(array, "%s          %s", left, right);
        LCD_WriteStringAtPos(array, 1, 0);
    }
    //If the right two IR sensors see see black, turn on only the left motor
    if (!PM1 && !PM2 && PM3 && PM4) {
        OC4RS = PR3 / 13; //Forward
        sprintf(left, "FWD");
        OC5RS = PR3 / 13; //Reverse
        sprintf(right, "REV");
        char array[16];
        sprintf(array, "%s          %s", left, right);
        LCD_WriteStringAtPos(array, 1, 0);
    }
    //If the left two IR sensors see see black, turn on only the right motor
    if (PM1 && PM2 && !PM3 && !PM4) {
        OC4RS = PR3 / 15; //Reverse
        sprintf(left, "REV");
        OC5RS = PR3 / 15; //Forward
        sprintf(right, "FWD");
        char array[16];
        sprintf(array, "%s          %s", left, right);
    }
    //If all IR sensors see black increment pmCount
    if (!PM1 && !PM2 && !PM3 && !PM4) {
        pmCount++;
        if (pmCount == 1) {
            LED_SetValue(0,1);
        } else if (pmCount == 2) {
            LED_SetValue(1,1);
        }
    }
    if (PM1 && PM2 && PM3 && PM4) { // Detect all white
        OC4RS = PR3 / 16; //Reverse
        sprintf(left, "REV");
        OC5RS = PR3 / 11; //Reverse
        sprintf(right, "REV");
        char array[16];
        sprintf(array, "%s          %s", left, right);
    }
    //If pmCount = 2, the bot has hit the second checkpoint and should stop
    if (pmCount == 2) {
        OC4RS = PR3 / 13.3; //Stop
        sprintf(left, "STP");
        OC5RS = PR3 / 13.3; //Stop
        sprintf(right, "STP");
        char array[16];
        sprintf(array, "%s          %s", left, right);
        tmrStart = 0;
    }

    LED_SetValue(7, PM1);
    LED_SetValue(6, PM2);
    LED_SetValue(5, PM3);
    LED_SetValue(4, PM4);
    
    IFS0bits.T3IF = 0; // clear interrupt flag
    IEC0bits.T3IE = 1; // enable interrupt
}

void Timer2Setup() {
    PR2 = (int)(((float)(TMR_TIME * PB_FRQ) / 256) + 0.5); //set period register, generates one interrupt every 3 ms
    TMR2 = 0;                           //    initialize count to 0
    T2CONbits.TCKPS = 0b111;                //    1:64 prescale value
    T2CONbits.TGATE = 0;                //    not gated input (the default)
    T2CONbits.TCS = 0;                  //    PCBLK input (the default)
    T2CONbits.ON = 1;                   //    turn on Timer1
    IPC2bits.T2IP = 7;                  //    priority
    IPC2bits.T2IS = 3;                  //    subpriority
    IFS0bits.T2IF = 0;                  //    clear interrupt flag
    IEC0bits.T2IE = 1;                  //    enable interrupt
    macro_enable_interrupts();
}

void Timer3Setup() {
    PR3 = (int)(((float)(TMR_TIME * PB_FRQ) / 256) / 4 ); //set period register, generates one interrupt every 3 ms
    TMR3 = 0;                           //    initialize count to 0
    T3CONbits.TCKPS = 0b111;                //    1:64 prescale value
    T3CONbits.TGATE = 0;                //    not gated input (the default)
    T3CONbits.TCS = 0;                  //    PCBLK input (the default)
    T3CONbits.ON = 1;                   //    turn on Timer1
    IPC3bits.T3IP = 7;                  //    priority
    IPC3bits.T3IS = 3;                  //    subpriority
    IFS0bits.T3IF = 0;                  //    clear interrupt flag
    IEC0bits.T3IE = 1;                  //    enable interrupt
    macro_enable_interrupts();
}

void activateServo(){
    
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
}