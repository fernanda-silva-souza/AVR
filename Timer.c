#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Timer.h"

// Inicializa Timer1 no modo CTC para gerar interrupções a cada 1 segundo
void timer1_ctc_init(void) {
	TCCR1B = 0x0C;           // Modo CTC, prescaler 256
	TCCR1A = 0;
	OCR1A = 62499;           // 1 Hz (1s)
	TIMSK1 |= (1 << OCIE1A); // Habilita interrupção no compare
}

// Inicializa Timer3 no modo CTC para piscar LED a 2 Hz
void timer3_ctc_init(void) {
	TCCR3B = 0x0C;           // Modo CTC, prescaler 256
	TCCR3A = 0;
	OCR3A = 15624;           // 2 Hz
	TIMSK3 |= (1 << OCIE3A); // Habilita interrupção no compare
}

// Para o Timer3 e desliga o LED
void timer3_stop(void) {
	TCCR3B = 0;
	TIMSK3 &= ~(1 << OCIE3A);
	PORTA &= ~(1 << PA0); // Desliga LED
}
