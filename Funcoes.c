#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include "Timer.h"
#include "Funcoes.h"

extern volatile char inatividade_segundos;
extern volatile char piscar_led;

// Esta função deve ser chamada sempre que houver interação do usuário.
// Ela zera o contador de inatividade e para o piscar do LED.
void reset_inatividade() {
	inatividade_segundos = 0;
	piscar_led = 0;
	timer3_stop();
}
