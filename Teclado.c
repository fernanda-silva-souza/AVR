#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Teclado.h"
#include "LCD.h"
#include "Timers.h"
#include "Serial.h"

// Incluir demais funções .h aqui depois

// Realiza o debounce
unsigned char debounce(char tecla) {
    unsigned char cont = 0;
    unsigned char ultima_tecla = 0;
    unsigned char tecla_atual = 0;

    while(cont < 7) {
        _delay_1ms();
        tecla_atual = (PINK & (1 << tecla));
        if (tecla_atual == ultima_tecla) {
            cont++;
        } 
	else {
            cont = 0;
            ultima_tecla = tecla_atual;
        }
    }
    return tecla_atual;
}

// Realiza leitura do teclado
unsigned char le_tecla() {
    unsigned char tecla_pressionada = 0;

    // Linha 1
    PORTK &= ~(1 << 0); // Nível lógico baixo na linha 1
    if (debounce(4) == 0) tecla_pressionada = '1';
    else if (debounce(5) == 0) tecla_pressionada = '2';
    else if (debounce(6) == 0) tecla_pressionada = '3';
    PORTK |= (1 << 0); // Nível lógico alto na linha 1

    // Linha 2
    PORTK &= ~(1 << 1); // Nível lógico baixo na linha 2
    if (debounce(4) == 0) tecla_pressionada = '4';
    else if (debounce(5) == 0) tecla_pressionada = '5';
    else if (debounce(6) == 0) tecla_pressionada = '6';
    PORTK |= (1 << 1); // Nível lógico alto na linha 2

    // Linha 3
    PORTK &= ~(1 << 2); // Nível lógico baixo na linha 3
    if (debounce(4) == 0) tecla_pressionada = '7';
    else if (debounce(5) == 0) tecla_pressionada = '8';
    else if (debounce(6) == 0) tecla_pressionada = '9';
    PORTK |= (1 << 2); // Nível lógico alto na linha 3

    // Linha 4
    PORTK &= ~(1 << 3); // Nível lógico baixo na linha 4
    if (debounce(4) == 0) tecla_pressionada = '*';
    else if (debounce(5) == 0) tecla_pressionada = '0';
    else if (debounce(6) == 0) tecla_pressionada = '#';
    PORTK |= (1 << 3); // Nível lógico alto na linha 4

    return tecla_pressionada;
}
