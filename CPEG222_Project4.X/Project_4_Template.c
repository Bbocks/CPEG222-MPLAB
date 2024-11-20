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

// Function Declarations
void initializePorts();
void pwmConfig();
void activateServo();
void Timer2Setup();
void Timer3Setup();
void setupLEDs(void);

int vals[] = {-1,-1,-1,-1};
int led_vals;
char left[3];
char right[3];


int main(void) {
    intializePorts();
    pwmConfig();
    Timer2Setup();
    Timer3Setup();
    setupLEDs();
    for (int i = 0; i < 0; i ++) {
        LED_SetValue(i,0);
    }
    
    LCD_WriteStringAtPos("Team: Mr. Krabs",0,0);
    LCD_WriteStringAtPos("STP          STP",1,0);
    
    while (TRUE) {
        //PS.. It might be a good idea to put this function in a timer ISR later on.
        activateServo();
    }
}




// Initialize ports on board
void intializePorts() {
    DDPCONbits.JTAGEN = 0; // Required to use Pin RA0 (connected to LED 0) as IO
    LED_Init();
    LCD_Init();
    SSD_Init();
    SWT_Init();
}

void pwmConfig() {
    
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
    
    IFS0bits.T2IF = 0; // clear interrupt flag
    IEC0bits.T2IE = 1; // enable interrupt
}

void __ISR(_TIMER_3_VECTOR) Timer3ISR(void) {
    IEC0bits.T3IE = 0; // disable interrupt
    
    if (sw7 == 1) {
        if (sw6 == 1) {
            OC4RS = PR2/13.3; //Stop
            //left = "STP";
            sprintf(left,"STP");
            LED_SetValue(4,0);
            LED_SetValue(5,0);
            LED_SetValue(6,0);
            LED_SetValue(7,0);
        } else if (sw6 == 0) {
            OC4RS = PR2/20; //Backwards
            //left = "REV";
            sprintf(left,"REV");
            LED_SetValue(6,1);
            LED_SetValue(7,1);
        }
    } else if (sw7 == 0) {
        if (sw6 == 1) {
            OC4RS = PR2/7.5; //Forwards
            //left = "FWD";
            sprintf(left,"FWD");
            LED_SetValue(4,1);
            LED_SetValue(5,1);
        } else if (sw6 == 0) {
            OC4RS = PR2/13.3; //Stop
            //left = "STP";
            sprintf(left,"STP");
            LED_SetValue(4,0);
            LED_SetValue(5,0);
            LED_SetValue(6,0);
            LED_SetValue(7,0);
        }
    }
    
    if (sw1 == 1) {
        if (sw0 == 1) {
            OC5RS = PR2/13.3; //Stop
            //right = "STP";
            sprintf(right,"STP");
            LED_SetValue(0,0);
            LED_SetValue(1,0);
            LED_SetValue(2,0);
            LED_SetValue(3,0);
        } else if (sw0 == 0) {
            OC5RS = PR2/20; //Backwards
            //right = "REV";
            sprintf(right,"REV");
            LED_SetValue(0,1);
            LED_SetValue(1,1);
        }
    } else if (sw1 == 0) {
        if (sw0 == 1) {
            OC5RS = PR2/7.5; //Forward
            //right = "FWD";
            sprintf(right,"FWD");
            LED_SetValue(2,1);
            LED_SetValue(3,1);
        } else if (sw0 == 0) {
            OC5RS = PR2/13.3; //Stop
            //right = "STP";
            sprintf(right,"STP");
            LED_SetValue(0,0);
            LED_SetValue(1,0);
            LED_SetValue(2,0);
            LED_SetValue(3,0);
        }
    }
    
    char array[16];
    sprintf(array,"%s          %s",left,right);
    LCD_WriteStringAtPos(array,1,0);
    
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