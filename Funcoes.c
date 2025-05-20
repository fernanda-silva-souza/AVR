#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Timer.h"
#include "Teclado.h"
#include "LCD.h"
#include "Serial.h"

extern char inatividade_segundos;
extern char piscar_led;

void reset_inatividade() {  // COLOCAR ESSA FUNÇÃO NO TECLADO
	inatividade_segundos = 0;
	piscar_led = 0;
	timer3_stop();
	//LED_PORT &= ~(1 << LED_PIN); // Garante LED apagado
}