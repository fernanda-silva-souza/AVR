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

extern volatile uint8_t sessao_ativa;
extern volatile char estado_caixa;
extern volatile char piscar_led;
extern volatile char inatividade_segundos;
extern volatile uint8_t sessao_encerrada_por_inatividade;
extern volatile uint8_t aguardando_resposta_saldo;
extern char saldo_recebido[12];
extern volatile signed char saque_aprovado;
extern volatile char existe;

const char mensagem_esperada[15] = {
	'N', 'a', 'o', ' ', 'a', 'u', 't', 'o', 'r', 'i', 'z', 'a', 'd', 'o'
};
char nomeagencia[32];
char nome[30];

volatile uint8_t contador_operacional = 0;
volatile uint8_t contador = 0;
volatile uint8_t USART_ReceiveBuffer;
volatile uint8_t asci_primeiro_byte = 0;
volatile uint8_t asci_segundo_byte = 0;
volatile uint8_t short_terceiro_byte = 0;
volatile uint8_t short_quarto_byte = 0;

ISR(USART0_RX_vect) {
	USART_ReceiveBuffer = UDR0;

	if (USART_ReceiveBuffer == 'S' && contador == 0) { 
		asci_primeiro_byte = USART_ReceiveBuffer;
		asci_segundo_byte = 0;
		short_terceiro_byte = 0;
		short_quarto_byte = 0;
		contador = 1;
		} else if (contador == 1) {
		asci_segundo_byte = USART_ReceiveBuffer;
		contador = 2;
		} else if (contador == 2) {
		short_terceiro_byte = USART_ReceiveBuffer;
		contador = 3;
		} else if (contador == 3) {
		short_quarto_byte = USART_ReceiveBuffer;
		contador = 4;
		} else {
		contador = 0;
		return; 
	}
	if (contador == 2) {
		if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'L') {
			caixa_liberado();
			estado_caixa = 1; // Liberado
			contador = 0;
			} else if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'T') {
			caixa_travado();
			estado_caixa = 0; // Travado
			lcd_limpar();
			lcd_string("FORA DE");
			lcd_comando(0xC0);
			lcd_string("OPERACAO");
			contador = 0; 
		}
	}
	if (contador == 3) {
		if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'S') {
			if (short_terceiro_byte == 'O') {
				saque_aprovado = 1;
				contador = 0; 
				} else if (short_terceiro_byte == 'I') {
				saque_aprovado = 0;
				contador = 0; 
			}
		}
		else if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'V') {
			uint8_t tamanho = short_terceiro_byte; 
			uint8_t i;
			if (tamanho > 11) {
				tamanho = 11;
			}
			for (i = 0; i < tamanho; i++) {
				while (!(UCSR0A & (1 << RXC0)));
				saldo_recebido[i] = UDR0;
			}
			saldo_recebido[i] = '\0';
			aguardando_resposta_saldo = 0;
			contador = 0; 
		}
		else if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'E') {
			char n = short_terceiro_byte;
			char iguais = 0;
			uint8_t j = 0;
			for (j = 0; j < n; j++){
				while (!(UCSR0A & (1 << RXC0)));
				nomeagencia[j] = UDR0;
			}
			nomeagencia[j] = '\0';
			for (j = 0; j < n; j++) {
				if (nomeagencia[j] == mensagem_esperada [j]) {
					iguais ++;
				}
			}
			if (iguais == n) {
				existe = 2; //nao existe esse usuario
			} else {
				existe = 1; //usuario existe
				for (j = 0; j < (n-3); j++) {
					nome[j] = nomeagencia [j];
					//lcd_limpar();
					//lcd_dado(nome[j]);
					//_delay_ms(2000);
				}
			}
			contador = 0; 
		}
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
	PORTA ^= (1 << PA0);
}
