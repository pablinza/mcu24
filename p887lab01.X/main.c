/*
 * File:   main.c
 * Author: Pablo Zarate A. pablinza@me.com
 * Created on April 5, 2024, 9:51 PM
 */
// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = ON       // Power-up Timer Enable bit (PWRT enabled)
#pragma config MCLRE = ON       // RE3/MCLR pin function select bit (RE3/MCLR pin function is MCLR)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF       // Brown Out Reset Selection bits (BOR enabled)
#pragma config IESO = OFF        // Internal External Switchover bit (Internal/External Switchover mode is enabled)
#pragma config FCMEN = OFF       // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is enabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)
// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)
#include <xc.h>

#define BUTpin PORTBbits.RB0 
#define LEDpin PORTEbits.RE2 
#define PORTstate0 0b010010; //Y Y
#define PORTstate1 0b100001; //R G
#define PORTstate2 0b100010; //R Y 
#define PORTstate3 0b001100; //G R
#define PORTstate4 0b010100; //Y R
#define time_1 3
#define time_2 10
char butreg = 0;
enum app {APP0, APP1, APP2, APP3 , APP4} appst;
volatile uint8_t tick1ms; 
void taskAPP(void);
void taskALERT(void);
void taskLED(void);
void __interrupt() isr()
{
    if(INTCONbits.T0IF) //Activa cada 1ms
     {
        INTCONbits.T0IF = 0;  //Limpia bandera
        TMR0 += 131; //Reinicia contador 
        tick1ms = 1; //Activa bandera 1ms
    }
}
void main(void) 
{
    OSCCONbits.IRCF = 0b111; //Internal Fosc=8MHz Tcy=0.5u
    while(OSCCONbits.HTS == 0) {};
    ANSEL = 0; //Deshabilita ANS0-7
    ANSELH = 0;//Deshabilita ANS8-13
    TRISBbits.TRISB0 = 1;
    TRISD = 0;
    TRISEbits.TRISE2 = 0;
    OPTION_REGbits.nRBPU = 0;
    // CONFIGURACION TIMER0 1MS //
    OPTION_REGbits.T0CS = 0;//Modo Termporizador
    OPTION_REGbits.PSA = 0; //Con prescala
    OPTION_REGbits.PS = 0b011; //Prescala 1:16
    TMR0 = 131; ////256-(time/((pre)*(4/Fosc))) time=0.001 seg
    //CONFIGURA ISR
    INTCONbits.T0IF = 0;
    INTCONbits.T0IE = 1;
    INTCONbits.GIE = 1;
    appst = APP0;
    while(1)
    {
        if(tick1ms)
        {
            tick1ms = 0;
            if(butreg != BUTpin) 
            {
                butreg = BUTpin;    
                if(butreg) appst = APP0;
            }
            if(butreg == 0) taskALERT();
            else taskAPP();
            taskLED();
        }
    }
}
void taskAPP(void)
{
    static unsigned int count;
    switch(appst)
    {
        case APP0: 
            count = 0;
            PORTD = PORTstate0;
            appst = APP1;
            break;
        case APP1:
            count ++;
            if(count > (time_1 * 1000U))
            {
                count = 0;
                PORTD = PORTstate1;
                appst = APP2;
            }
            break;
        case APP2:
            count ++;
            if(count > (time_2 * 1000U))
            {
                count = 0;
                PORTD = PORTstate2;
                appst = APP3;
            }
            break;
        case APP3:
            count ++;
            if(count > (time_1 * 1000U))
            {
                count = 0;
                PORTD = PORTstate3;
                appst = APP4;
            }
            break;
        case APP4:
            count ++;
            if(count > (time_2 * 1000U))
            {
                count = 0;
                PORTD = PORTstate4;
                appst = APP1;
            }
    }
}
void taskALERT(void)
{   
    static unsigned int count = 0;
    static char state = 0;
    switch(state)
    {
        case 0: 
            count = 0;
            state = 1;
            break;
        case 1:
            count++;
            if(count > 249)
            { 
                count = 0;
                state = 2;
                PORTD = 0;
            }
            break;
        case 2:
            count++;
            if(count > 249)
            { 
                count = 0;
                state = 1;
                PORTD = PORTstate0;
            }        
    }
    
}
void taskLED(void) //Blink led task
{
    static uint16_t tcnt = 0;
    if(tcnt++ > 999) 
    {
        tcnt = 0;
        LEDpin = 1;
    }
    if(tcnt == 200) LEDpin = 0;
}
