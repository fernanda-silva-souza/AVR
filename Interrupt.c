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

	// Revised byte accumulation logic
	if (USART_ReceiveBuffer == 'S' && contador == 0) { // Treat 'S' as a command starter if no command is in progress
		asci_primeiro_byte = USART_ReceiveBuffer;
		asci_segundo_byte = 0; // Clear subsequent bytes for a new command
		short_terceiro_byte = 0;
		short_quarto_byte = 0;
		contador = 1;
		} else if (contador == 1) { // Expecting the second byte
		asci_segundo_byte = USART_ReceiveBuffer;
		contador = 2;
		} else if (contador == 2) { // Expecting the third byte
		short_terceiro_byte = USART_ReceiveBuffer;
		contador = 3;
		} else if (contador == 3) { // Expecting the fourth byte (not used by SSO/SSI but good for other commands)
		short_quarto_byte = USART_ReceiveBuffer;
		contador = 4;
		} else {
		// Unexpected byte or command too long for current simple parsing, reset
		contador = 0;
		// Optionally, you might want to clear asci_primeiro_byte etc. here too
		// or handle this as an error.
		return; // Exit ISR early if state is unknown
	}

	// Command Interpretation
	// Check for commands only when enough bytes have been potentially received.

	if (contador == 2) { // Check for 2-byte commands like 'SL', 'ST'
		if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'L') {
			caixa_liberado();
			estado_caixa = 1; // Liberado
			contador = 0;     // Reset for next command
			} else if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'T') {
			caixa_travado();
			estado_caixa = 0; // Travado
			lcd_limpar();
			lcd_string("FORA DE");
			lcd_comando(0xC0);
			lcd_string("OPERACAO");
			contador = 0; // Reset for next command
		}
	}

	// Check for 3-byte commands like 'SSO', 'SSI'
	// Also the start of 'SV' command (S, V, length)
	if (contador == 3) {
		if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'S') {
			if (short_terceiro_byte == 'O') {
				saque_aprovado = 1;
				contador = 0; // Reset for next command
				} else if (short_terceiro_byte == 'I') {
				saque_aprovado = 0;
				contador = 0; // Reset for next command
			}
		}
		// Tratamento da resposta de saldo: 'S''V' n "Saldo"
		else if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'V') {
			uint8_t tamanho = short_terceiro_byte; // The third byte is the length
			uint8_t i;

			// Ensure not to overflow saldo_recebido
			if (tamanho > 11) {
				tamanho = 11;
			}

			for (i = 0; i < tamanho; i++) {
				while (!(UCSR0A & (1 << RXC0))); // Wait for data to be received
				saldo_recebido[i] = UDR0;
			}
			saldo_recebido[i] = '\0';
			aguardando_resposta_saldo = 0;
			contador = 0; // Reset for next command
		}
		// If it was a 3-byte command not matching above, and not SV,
		// you might want to reset contador if it's an unknown 3-byte command.
		// else { contador = 0; } // Optional: if it's not SSO/SSI/SV start
	}

	// If a command is longer than 3 bytes and not 'SV' + data,
	// the current logic might need extension or a reset for contador if (contador == 4 && no_match)

	// Note: If a command (e.g., SL) is processed and contador is reset to 0,
	// subsequent checks in the same ISR call (like for contador == 3) will not trigger, which is correct.
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


