/*===================================CPEG222====================================
 * Program:      Project 4 Template
 * Authors:     Robert Freeman
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

#define sw0 PORTFbits.RF3
#define sw1 PORTFbits.RF5
#define sw6 PORTBbits.RB10
#define sw7 PORTBbits.RB9

// Function Declarations
void initializePorts();
void pwmConfig();
void activateServo();
void Timer2Setup();
void Timer3Setup();

int vals[] = {0,0,0,0};


int main(void) {
    intializePorts();
    pwmConfig();
    Timer2Setup();
    Timer3Setup();
    
    while (TRUE) {
        //PS.. It might be a good idea to put this function in a timer ISR later on.
        activateServo();
    }
}




// Initialize ports on board
void intializePorts() {
    LED_Init();
    LCD_Init();
    SSD_Init();
    SWT_Init();
}

void pwmConfig() {
    
    // configure Timer X (select the timer, replace X with desired timer number)
    
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
    
    
    // Configure Output Compare Module 4
    
    OC4CONbits.OCM = 6;      // PWM mode on OC4; Fault pin is disabled
    OC4CONbits.OCTSEL = PR2;   // Select the timer to use as a clock source
    OC4RS = PR2/13.3;//OC4RS is some fraction of the Period
    OC4R = OC4RS;
    OC4CONbits.ON = 1;       // Start the OC4 module
    
    //Do The same for OC5**************************
    OC5CONbits.OCM = 6;      // PWM mode on OC5; Fault pin is disabled
    OC5CONbits.OCTSEL = PR2;   // Select the timer to use as a clock source
    OC5RS = PR2/13.3;//OC5RS is some fraction of the Period
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
    
    vals[0]++;
    if (vals[0]%10==0) {
        vals[1]++;
    } 
    if (vals[1]%10==0) {
        vals[2]++;
    } 
    if (vals[2]%10==0) {
        vals[3]++;
    } 
    SSD_WriteDigits(vals[0]%10,vals[1]%10,vals[2]%10,vals[3]%10,0,1,0,0)
    
    IFS0bits.T2IF = 0; // clear interrupt flag
    IEC0bits.T2IE = 1; // enable interrupt
}

void __ISR(_TIMER_3_VECTOR) Timer3ISR(void) {
    IEC0bits.T3IE = 0; // disable interrupt
    
    if (sw1 == 1) {
        if (sw0 == 1) {
            OC4RS = PR2/13.3; //Stop
        } else if (sw0 == 0) {
            OC4RS = PR2/20; //Backwards
        }
    } else if (sw1 == 0) {
        if (sw0 == 1) {
            OC4RS = PR2/10; //Forwards
        } else if (sw0 == 0) {
            OC4RS = PR2/13.3; //Stop
        }
    }
    
    if (sw7 == 1) {
        if (sw6 == 1) {
            OC5RS = PR2/13.3; //Stop
        } else if (sw6 == 0) {
            OC5RS = PR2/20; //Backwards
        }
    } else if (sw7 == 0) {
        if (sw6 == 1) {
            OC5RS = PR2/10; //Forward
        } else if (sw6 == 0) {
            OC5RS = PR2/13.3; //Stop
        }
    }
    
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
    PR3 = (int)(((float)(TMR_TIME * PB_FRQ) / 256) + 0.5); //set period register, generates one interrupt every 3 ms
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
    if(SWT_GetValue(6)){
        //If Switch 6 is on, move servo...
        //Replace X with your Timer Number
        OC4RS = (int) PR2 / 25;
    }else{
        //If Switch 6 is off, stop moving servo...
        //Replace X with your Timer Number
        OC4RS = (int) PR2 / 13.8; 
    }
}
