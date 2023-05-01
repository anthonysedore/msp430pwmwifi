#include <msp430.h>
#include <stdio.h>
#include <string.h>

#define MAX_MESSAGE_LENGTH 12

//Variables for storing UART characters and tracking position within character array
char message[MAX_MESSAGE_LENGTH];
char character;
unsigned int msg_counter = 0;
unsigned int msg_length = 0;
unsigned int new_msg = 0;

unsigned int DutyCycle = 0;          //Duty Cycle initially set to 0

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                 // Stop watchdog timer

    // Configure UART
     UCA1CTLW0 |= UCSWRST;                     // Put eUSCI in reset
     UCA1CTLW0 |= UCSSEL__SMCLK;
     // Baud Rate calculation
     UCA1BR0 = 8;                              // 1000000/115200 = 8.68
     UCA1MCTLW = 0xD600;                       // 1000000/115200 - INT(1000000/115200)=0.68
                                               // UCBRSx value = 0xD6 (See UG)

     // Configure UART pins
     P4SEL1 &= ~BIT2; //RX CONFIG
     P4SEL0 |= BIT2;  //Pin 4.2
     P4SEL1 &= ~BIT3; //TX CONFIG
     P4SEL0 |= BIT3;  //Pin 4.3

     P4DIR |= 0xC1; //Set P4.0, 4.6, 4.7 as output,
     P4OUT &= ~0xC1; // clear P4 pins
     P4OUT |= 0x40; //Turn P4.6 ON, to make motor spin IN1 HIGH and IN2 LOW
     TB0CTL = TBSSEL__ACLK + MC__UP + TBCLR;      // ACLK, upmode, clear
     TB0CCR0 = 1000;                              // PWM Period, Timer resets when = TB0CCR0, PWM is 32.75 Hz, optimal is 50-100 Hz range for h bridge, shrug
     TB0CCR1 = 0;                                 // Duty Length, TB0CCR1/TB0CCR0 = duty cycle
     TB0CCTL1 = CCIE;                             // CCR0 interrupt enabled
     TB0CCTL1&=~(CCIFG);                          // Clear interrupt flag
     TB0CCTL0 = CCIE;                             // CCR1 interrupt enabled
     TB0CCTL0&=~(CCIFG);                          // Clear interrupt flag

     // Disable the GPIO power-on default high-impedance mode
     // to activate previously configured port settings
     PM5CTL0 &= ~LOCKLPM5;

     UCA1CTLW0 &= ~UCSWRST;                    // Initialize eUSCI, Turn off software reset for UART pins

     UCA1IE |= UCRXIE;  //Enable UART RX interrupts
     __enable_interrupt();

     while(1)
     {
         if(new_msg)
         {
             sscanf(message, "%d", &DutyCycle); //Converts char array of key presses into integer
             if (strcmp(message, "FORWARD") == 0) //If message reads forward, IN 1 ON IN 2 OFF
             {
                 P4OUT |= BIT7; //4.6 ON
                 P4OUT &= BIT8; //4.7 OFF
             }
             else if (strcmp(message, "BACKWARD") == 0) //If message reads backward, IN 1 OFF IN 2 ON
             {
                 P4OUT &= BIT7; //4.6 OFF
                 P4OUT |= BIT8; //4.7 ON
             }
             else if (strcmp(message, "OFF") == 0)  //If message reads OFF, IN 1 OFF IN 2 ON
             {
                 P4OUT ^= BIT7; //Toggles 4.6
             }
             else if (strcmp(message, "ON") == 0) //If message reads ON, toggle IN 1
             {
                 P4OUT ^= BIT7; //Toggles 4.6
             }
             else if (strcmp(message, "PWM") == 0) //the esp32 send out PWM before sending number, only need number
             {
                 ;
             }
             else if (DutyCycle>=0) //Only Activates if the message sent was a number
             {
                 if (DutyCycle >= 100)
                 {  //If Input Value was bigger than 100, set duty cycle to 100
                     DutyCycle = 100;
                     TB0CCR1 = 1001; //Just to set TB0CCR1 more than TB0CCR0, if they equal there becomes unpredictable errors with interrupts
                 }
                 else
                 {
                     TB0CCR1 = DutyCycle*10; //Duty Cycle, ex 50 convert to 500 so TB0CCR1/TB0CCR0 (TB0CCR0 = 1000) = 0.50 <-desired duty cycle
                 }
             }
             new_msg = 0; //Set flag down for new message
         }
     }
     return 0;
}

#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
{
    character = UCA1RXBUF; //Reads bytes from RX Buffer and turns off interrupt flag
        if (character >= 'A' && character <= 'z') //Only reads letters
        {
            message[msg_counter++] = character; //Append character to char array of message
        }
        else if (character >= '0' && character <= '9') //Only reads numbers
        {
            message[msg_counter++] = character; //Append number to char array of message
        }
        else if (character == '\n') //When character = newline, it ends the message
        {
            message[msg_counter] = '\0'; //Terminates string character
            msg_length = msg_counter - 1; //Records length of message array so doesn't cause unpredictable behavior
            msg_counter = 0; //resets message position
            new_msg = 1; //Set flag for new message to be read in main loop
        }
}

//When timer = TB0CCR0
#pragma vector=TIMER0_B0_VECTOR
__interrupt void TMR0 ()
{
    P4OUT |= 1; //Turns On P4.0
    TB0CCTL0 &= ~(CCIFG);           // Clear interrupt flag
}

//When timer = TB0CCR1
#pragma vector=TIMER0_B1_VECTOR
__interrupt void TMR1 ()
{
    P4OUT &= ~1; //Turn off P4.0
    TB0CCTL1 &= ~(CCIFG);           // Clear interrupt flag
}
