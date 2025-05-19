#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "Teclado.h"
#include "LCD.h"
#include "Timer.h"
#include "Serial.h"

extern volatile char bloqueado;
extern volatile char contador;
extern volatile char liberado;
extern volatile char contador_segundos;
volatile uint8_t USART_ReceiveBuffer;
volatile uint8_t asci_primeiro_byte = 0;		// mensagens de 1 byte
volatile uint8_t asci_segundo_byte = 0;		// mensagens de 2 bytes
volatile uint8_t short_terceiro_byte = 0;		// mensagens de 3 bytes
volatile uint8_t short_quarto_byte = 0;		// mensagens de 4 bytes

ISR(USART0_RX_vect){
	USART_ReceiveBuffer = UDR0;
	contador++;
	
	if (USART_ReceiveBuffer == 'S') // sempre inicia com 'S', e colocamos contador 1 como primeiro byte recebido
	{
		contador = 1;
		asci_primeiro_byte = USART_ReceiveBuffer;
	}
	else if (contador == 2)
	{
		asci_segundo_byte = USART_ReceiveBuffer;
	}
	else if (contador == 3)
	{
		short_terceiro_byte = USART_ReceiveBuffer;
	}
	else if (contador == 4)
	{
		short_quarto_byte = USART_ReceiveBuffer;
	}
	if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'L') // Liberação do Terminal
	{
		liberado = 1;
	}
	if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'T') // Travamento do Terminal
	{
		bloqueado++;
	}
}

ISR(TIMER1_COMPA_vect) {
	contador_segundos++;

	if (contador_segundos == 120) {
		caixa_operando_normalmente();
		contador_segundos = 0;
	}
	
}