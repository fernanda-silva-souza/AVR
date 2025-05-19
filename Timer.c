#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Timer.h"
#include "Teclado.h"
#include "LCD.h"
#include "Serial.h"

void timer1_ctc_init(void){  // Timer 1 configurado no modo CTC
	TCCR1B = 0x0B;   // Prescaler = 256 (CS12 = 0, CS11 = 1, CS10 = 1) WGM12 = 1 para CTC
	TCCR1A = 0;
	OCR1A = 62499;           // Valor para gerar 2 Hz com f_clk = 16 MHz e prescaler = 256
	TIMSK1 |= (1 << OCIE1A); // ao alcan?ar o valor, interrompe
	sei();
}