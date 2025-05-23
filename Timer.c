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

void timer1_stop(void) {
	TCCR1B = 0;
	TIMSK1 &= ~(1 << OCIE1A);
}

// Timer3 — pisca o LED a 2 Hz nos últimos 12s
void timer3_ctc_init(void) {
	TCCR3B = 0x0C;     // CTC mode, prescaler 256
	TCCR3A = 0;
	OCR3A = 15624; // PISCA 2X/SEGUNDO		//31249;// 2 Hz (16MHz / 256 / (31249 + 1) = 2 Hz)
	TIMSK3 |= (1 << OCIE3A); // Habilita interrupção no compare
}

// Para o timer3 (piscar LED)
void timer3_stop(void) {
	TCCR3B = 0;
	TIMSK3 &= ~(1 << OCIE3A);
	PORTA &= ~(1 << PA0);//desliga led quando encerra sessão
}

// Timer2 — delay de 1ms
void delay_1ms(void) {
	TCCR2B = 0x0B;     // CTC mode, prescaler 64
	TCCR2A = 0;//modo normal
	OCR2A = 124;     // Ticks
	while (!(TIFR2 & (1<<OCF2A)));
	TIFR2|=  (1 << OCF2A); //limpa flag de comparacao
}

// Timer2 — delay de 2ms
void delay_2ms(void) {
	TCCR2B = 0x0B;     // CTC mode, prescaler 64
	TCCR2A = 0;	//modo normal
	OCR2A = 249;     //Ticks
	while (!(TIFR2 & (1<<OCF2A)));
	TIFR2|=  (1 << OCF2A); //limpa flag de comparacao
}

// Timer2 — delay de 2ms
void delay_10ms(void) {
	TCCR2B = 0x0D;     // CTC mode, prescaler 1024
	TCCR2A = 0;	//modo normal
	OCR2A = 77;     //Ticks
	while (!(TIFR2 & (1<<OCF2A)));
	TIFR2|=  (1 << OCF2A); //limpa flag de comparacao
}


/*
TIMERS PARA USO DO LCD
// Timer4 — delay de 40ms
void delay_40ms(void) {
	TCCR4B = 0x0D;     // CTC mode, prescaler 1024
	TCCR4A = 0;	//modo normal
	OCR4A = 311;     //Ticks
	while (!(TIFR4 & (1<<OCF4A)));
	TIFR4 |=  (1 << OCF4A); //limpa flag de comparacao
}

// Timer4 — delay de 15ms
void delay_15ms(void) {
	TCCR4B = 0x0D;     // CTC mode, prescaler 1024
	TCCR4A = 0;	//modo normal
	OCR4A = 116;     //Ticks
	while (!(TIFR4 & (1<<OCF4A)));
	TIFR4 |=  (1 << OCF4A); //limpa flag de comparacao
}


*/
