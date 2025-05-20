#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Timer.h"
#include "Teclado.h"
#include "LCD.h"
#include "Serial.h"

void timer1_ctc_init(void){  // Timer 1 configurado no modo CTC
	TCCR1B = 0x0C;   // Prescaler = 256 (CS12 = 0, CS11 = 1, CS10 = 1) WGM12 = 1 para CTC
	TCCR1A = 0;
	OCR1A = 62499;           // Valor para gerar 2 Hz com f_clk = 16 MHz e prescaler = 256
	TIMSK1 |= (1 << OCIE1A); // ao alcan�ar o valor, interrompe
}

void timer3_ctc_init(void){  // Timer 3 configurado no modo CTC
	TCCR3B = 0x0B;   // Prescaler = 256 (CS12 = 0, CS11 = 1, CS10 = 1) WGM12 = 1 para CTC
	TCCR3A = 0;
	OCR3A = 62499;           // Valor para gerar 2 Hz com f_clk = 16 MHz e prescaler = 256
	TIMSK3 |= (1 << OCIE3A); // ao alcan�ar o valor, interrompe
}

void timer3_stop(void) {
	TCCR3B = 0;
	TIMSK3 &= ~(1 << OCIE3A);
}

