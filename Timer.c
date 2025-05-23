#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Timer.h"

// Timer1 — controla o tempo de inatividade (1s por interrupção)
void timer1_ctc_init(void) {
	TCCR1B = 0x0C;     // CTC mode, prescaler 256
	TCCR1A = 0;
	OCR1A = 62499;     // 1 segundo (16MHz / 256 / (62499 + 1) = 1 Hz)
	TIMSK1 |= (1 << OCIE1A); // Habilita interrupção no compare
}

// Timer3 — pisca o LED a 2 Hz nos últimos 12s
void timer3_ctc_init(void) {
	TCCR3B = 0x0C;     // CTC mode, prescaler 256
	TCCR3A = 0;
	OCR3A = 31249;     // 2 Hz (16MHz / 256 / (31249 + 1) = 2 Hz)
	TIMSK3 |= (1 << OCIE3A); // Habilita interrupção no compare
}

// Para o timer3 (piscar LED)
void timer3_stop(void) {
	TCCR3B = 0;
	TIMSK3 &= ~(1 << OCIE3A);
}

