#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>

#include "Teclado.h"
#include "LCD.h"
#include "Timer.h"
#include "Serial.h"
#include "Funcoes.h"

extern volatile char saque_aprovado;
extern volatile uint8_t sessao_ativa;
extern volatile char estado_caixa;
extern volatile char piscar_led;
extern volatile char inatividade_segundos;
extern volatile uint8_t sessao_encerrada_por_inatividade;
extern volatile uint8_t aguardando_resposta_saldo;
extern char saldo_recebido[12];
extern volatile char saque_aprovado;

volatile uint8_t contador_operacional = 0;
volatile uint8_t contador = 0;
volatile uint8_t USART_ReceiveBuffer;
volatile uint8_t asci_primeiro_byte = 0;
volatile uint8_t asci_segundo_byte = 0;
volatile uint8_t short_terceiro_byte = 0;
volatile uint8_t short_quarto_byte = 0;

// Interrupção de recepção serial USART
ISR(USART0_RX_vect) {
	USART_ReceiveBuffer = UDR0;
	contador++;

	if (USART_ReceiveBuffer == 'S') {
		contador = 1;
		asci_primeiro_byte = USART_ReceiveBuffer;
		} else if (contador == 2) {
		asci_segundo_byte = USART_ReceiveBuffer;
		} else if (contador == 3) {
		short_terceiro_byte = USART_ReceiveBuffer;
		} else if (contador == 4) {
		short_quarto_byte = USART_ReceiveBuffer;
	}

	// Interpretação dos comandos recebidos
	if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'L') {
		caixa_liberado();
		estado_caixa = 1; // Liberado
		} else if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'T') {
		caixa_travado();
		estado_caixa = 0; // Travado
		lcd_limpar();
		lcd_string("FORA DE");
		lcd_comando(0xC0);
		lcd_string("OPERACAO");
	}
	
	// Tratamento da resposta de saldo: 'S''V' n "Saldo"
	if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'V' && contador == 3) {
		uint8_t tamanho = short_terceiro_byte;
		uint8_t i;

		for (i = 0; i < tamanho && i < 11; i++) {
			while (!(UCSR0A & (1 << RXC0)));
			saldo_recebido[i] = UDR0;
		}
		saldo_recebido[i] = '\0';
		aguardando_resposta_saldo = 0;
	}
	
	// Interpretação dos comandos recebidos
	if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'S' && short_terceiro_byte == 'O') {
		saque_aprovado = 1;
		} else if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'S' && short_terceiro_byte == 'I') {
		saque_aprovado = 0;
		}

}

// Temporizador para inatividade (30s máx, LED pisca a partir de 18s)
ISR(TIMER1_COMPA_vect) {
	inatividade_segundos++;

	if (inatividade_segundos >= 18 && !piscar_led) {
		piscar_led = 1;
		timer3_ctc_init(); // Inicia piscar LED
	}

	if (inatividade_segundos >= 30) {
		timer3_stop();
		inatividade_segundos = 0;
		piscar_led = 0;

		sessao_ativa = 0;
		sessao_encerrada_por_inatividade = 1;
	}

	
	// Envio periódico de status operacional
	if (estado_caixa == 1) {
		contador_operacional++;
		if (contador_operacional >= 30) {  // A cada 60 segundos
			caixa_operando_normalmente();
			contador_operacional = 0;
		}
	}
}

// Interrupção do Timer 3 (piscar LED 2 Hz)
ISR(TIMER3_COMPA_vect) {
	// Piscar LED aqui se for implementado fisicamente
	// Exemplo:
	// PORTB ^= (1 << PB7);
}


