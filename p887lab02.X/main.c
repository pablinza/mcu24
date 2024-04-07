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

#define LEDpin PORTEbits.RE2 
#define FANpin PORTCbits.RC0 
volatile uint8_t tick1ms;
uint16_t adcval = 0;
char temp = 0;
char adcOK = 0;
void taskLED(void);
void taskADC(void);
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
    ANSEL = 0;
    ANSELH = 0;
    TRISAbits.TRISA0 = 1;
    TRISCbits.TRISC0 = 0;
    TRISEbits.TRISE2 = 0; //Salida LED
    FANpin = 0;
    OPTION_REGbits.nRBPU = 0; //Activa pull-ups PORT
    // CONFIGURACION TIMER0 1MS //
    OPTION_REGbits.T0CS = 0;//Modo Termporizador
    OPTION_REGbits.PSA = 0; //Con prescala
    OPTION_REGbits.PS = 0b011; //Prescala 1:16
    TMR0 = 131; ////256-(time/((pre)*(4/Fosc))) time=0.001 seg
    //CONFIGURA ADC CANAL 0
    ANSELbits.ANS0 = 1;
    ADCON0bits.CHS = 0b0000;
    ADCON0bits.ADCS = 0b10; //TAD=4us (Fosc/32)
    //CONFIGURA ISR //
    INTCONbits.T0IF = 0;
    INTCONbits.T0IE = 1;
    INTCONbits.GIE = 1;
    while(1)
    {
        if(tick1ms)
        {
            tick1ms = 0;
            taskLED();
            taskADC();
            if(adcOK)
            {
                adcOK = 0;
                if(adcval > 102) temp = (char) (adcval - 102U); //positivo
                else temp = 0; //negativo
                temp = temp >> 1; //Divido entre 2bit por C
                if(temp < 28) FANpin = 0;
                if(temp > 30) FANpin = 1;
            }
        }
    }
}
void taskADC(void)
{
    static uint16_t count = 0;
    static char state = 0;
    count++;
    switch(state)
    {
        case 0: 
            ADCON0bits.ADON = 1; //espera TACQ = 12u
            state = 1;
            break;
        case 1:
            ADCON0bits.GO = 1;
            state = 2;
            break;
        case 2: 
            if(ADCON0bits.GO == 0)
            {  
                adcval = ADRESH; //Recupera los 8-bits de mas peso
                adcval = adcval << 8; //Desplaza 8 bits a la izquierda
                adcval = adcval | ADRESL; //Recupera los 2-bits de menor peso
                adcval = adcval >> 6; //Desplaza 6 bits 
                ADCON0bits.ADON = 0;
                adcOK = 1;
                state = 3;
            }
            break;
        case 3:
            if(count > 499)
            {
                count = 0;
                state = 0;
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
