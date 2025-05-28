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
char saldo_recebido[12];
extern volatile signed char saque_aprovado;

extern char saldoformatado[15];

const char mensagem_esperada[15] = {
	'N', 'a', 'o', ' ', 'a', 'u', 't', 'o', 'r', 'i', 'z', 'a', 'd', 'o'
};
char nomeagencia[32];
char nome[30];
extern volatile char existe = 0;

char diameshoramin[4];
volatile char hora_recebida = 0;
volatile char dia = 0;
volatile char hora = 0;
volatile char min = 0;
volatile char seg = 0;
extern volatile char fora_de_funcionamento;

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
		} else if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'H') {
		uint8_t j = 0;
		for (j = 0; j < 4; j++){
			while (!(UCSR0A & (1 << RXC0)));
			diameshoramin[j] = UDR0;
			//lcd_limpar();
			//exibir_numero_lcd_com_dado(diameshoramin[j]);
			//_delay_ms(2000);
		}
		diameshoramin[j] = '\0';
		dia = diameshoramin[0];
		hora = diameshoramin[2];
		min = diameshoramin[3];
		caixa_data_hora();
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
			
			char k = 0;
			saldoformatado[k++]='R';
			saldoformatado[k++]='$';
			for (i = 0; i < (tamanho-2); i++) {
				saldoformatado[k++] = saldo_recebido[i];
			}
			saldoformatado[k++]=',';
			saldoformatado[k++] = saldo_recebido[tamanho-2];
			saldoformatado[k++] = saldo_recebido[tamanho-1];
			saldoformatado[k++] = '\0';

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
					nome[j] = nomeagencia[j];
					//lcd_limpar();
					//lcd_dado(nome[j]);
					//_delay_ms(2000);
				}
			}
			contador = 0; 
		}
	}
}

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

	if (estado_caixa == 1) {
		contador_operacional++;
		if (contador_operacional >= 20) {  // A cada 60 segundos
			caixa_operando_normalmente();
			contador_operacional = 0;
		}
	}
	
	if (hora_recebida == 1) {
		seg++;
		if (seg >= 60) {
			seg = 0; 
			min++;
			//lcd_limpar();
			//exibir_numero_lcd_com_dado(min);
			if (min >= 60) {
				min = 0;
				hora++;
				//lcd_limpar();
				//exibir_numero_lcd_com_dado(hora);
				if (hora >= 24) {
					hora = 0;
					dia++;
				}
			}
		}
		if (hora >= 8 && hora < 20) {
			fora_de_funcionamento = 0;
		}
		else {
			fora_de_funcionamento = 1;
		}
	}
}

// Interrupção do Timer 3 (piscar LED 2 Hz)
ISR(TIMER3_COMPA_vect) {
	PORTA ^= (1 << PA0);
}
