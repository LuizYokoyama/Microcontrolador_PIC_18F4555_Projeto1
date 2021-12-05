/* 
 * File:   main.c
 * Author: Luiz Felix
 *
 * Created on 14 de Dezembro de 2020, 11:05
 */


#define _XTAL_FREQ 4000000
#include <xc.h>

#define RS LATD2 // pinos do LCD
#define EN LATD3
#define D4 LATD4
#define D5 LATD5
#define D6 LATD6
#define D7 LATD7

#pragma config FOSC = HS // Oscilador externo
#pragma config WDT = OFF        // Watchdog Timer desligado
#pragma config MCLRE = OFF      // Master Clear desligado

#include "lcd.h"
#include <stdio.h>
#include <stdlib.h>

char linha1[16];
char linha2[16];
int contador = 0;
float temperatura = 0.0;
int statusAnterior = 3; // começa com 3 para entrar no if do InterpretaTemperatura
int statusAtual = 0; // 0=baixa,1=adequada,2=alta
int contadorLed = 2; // variável auxiliar para o led piscar com 100 ms

void setupADC(void);
void setupTIMER0(void);
void interrupcao_ligada(void);
void interrupcao_desligada(void);
void __interrupt() interrupcao(void);
char* InterpretaTemperatura(float temp);

void main(void) {

    TRISC = 0;  // define a porta C como saída para controlar o motor CC
                // RC1 = Enable, RC4 = IN1, RC5 = IN2
    
    TRISD = 0;	// preciso configurar a porta D com todos os pinos de saída
    Lcd_Init(); // antes de inicializar o LCD 
    
    setupTIMER0();
    setupADC();
    
    while(1) {
        ADCON0bits.GO = 1;                  //Inicia a conversão AD
        while(!ADCON0bits.GODONE){}         //Aguarda o fim da conversão
        contador = (ADRESH*0x100)+ADRESL;   //Transfere valor p/ variável
        temperatura = ((1.5*100*contador)/1023.0); //Calcula temperatura
        // LM35: 10 mV por Graus Celcius        
               
        sprintf(linha1, "Temperatura: %3.1f    ",temperatura); // Grava texto em linha1
        sprintf(linha2, InterpretaTemperatura(temperatura)); // Grava texto em linha2  
        statusAnterior=statusAtual; //Salva o status atual da temperatura 
                                    //0=Baixa, 1=Adequada, 2=Alta
        Lcd_Set_Cursor(1,1);      // Posiciona o cursor na linha 1, caractere 1
        Lcd_Write_String(linha1); // Escreve texto de linha1 no LCD
        Lcd_Set_Cursor(2,1);      // Posiciona o cursor na linha 2, caractere 1
        Lcd_Write_String(linha2); // Escreve texto de linha2 no LCD
    }
}

void interrupcao_ligada(void){
    GIE = 1;    // habilita interrupção global
    TMR0IE = 1; // interrupção do timer 0
}

void interrupcao_desligada(void){
    GIE = 0;
    TMR0IE = 0;
}

void setupTIMER0(void){
    T08BIT = 0;     // Modo 16 bits
    T0CS = 0;       // Source do clock(operando como temporizador, e não como contador
    PSA = 1;        // Desabilita Prescaler
    TMR0H = 0x3C;   // Conta 100 mil vezes, freq = 4MHz, tempo de instr = 1 us
    TMR0L = 0xAF;   // então 1 us * 100 000 = 0,1 seg = 100 ms
    TMR0ON = 1;     // Liga o timer
}

void __interrupt() interrupcao(void){
    if(TMR0IF) { // Caso a flag do temporizador esteja ativa
       contadorLed--;
       if(contadorLed==0) // se o contador for zero, aí sim inverte o LED
       {
           contadorLed=2;
           LATDbits.LD0 = !LATDbits.LATD0;// Inverte pino D0
       }       
       TMR0H = 0x3C;// Começa a contar de 15535 (9E57)
       TMR0L = 0xAF;// até 65535 (conta 50 mil vezes)
       TMR0IF = 0;  // Flag do timer 0 em 0} 
    }
}

void setupADC(void){
    
    TRISA = 0b00000001;  // Habilita pino A0 como entrada
 
    ADCON2bits.ADCS = 0b110; // clock = Fosc/64
    ADCON2bits.ACQT = 0b010; // tempo aquisição = 4 Tad
    ADCON2bits.ADFM = 0b1;   // justificado à direita
    
    ADCON1bits.VCFG = 0b01;  // Tensões de referência: Vss e Pino AN3 (1.5 Volts)
    // Saída máxima do LM35 resulta na saída máxima do AD
    
    ADCON0bits.CHS = 0b0000; // Canal AN0
    ADCON0bits.ADON = 1;     // Conversor AD ligado
}

char* InterpretaTemperatura(float temp){ // tem tipo char* pq o retorno dessa função é a mensagem
    if(temp<20)
    {
        statusAtual=0;
         if(statusAnterior!=statusAtual) // se for mudar de mensagem,
        {                                // limpa o LCD e desliga motor CC
            Lcd_Clear();
            LATC=0b00000000;      // desliga motor CC
            interrupcao_ligada(); // liga interrupção do timer 0
        }
        return "Muito Baixa";
    }
    else if(temp<80)
    {
        statusAtual=1;
        if(statusAnterior!=statusAtual)
        {
            Lcd_Clear();
            LATC=0b00000000;         // desliga motor CC
            interrupcao_desligada(); // desliga interrupção
        }
        return "Valor Adequado";
    }
    statusAtual=2;
    if(statusAnterior!=statusAtual) 
    {
        Lcd_Clear();
        LATC=0b00010010;         // liga motor CC
        interrupcao_desligada(); // desliga interrupção
    }
    return "Muito Alta";       
}
