#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#include "Teclado.h"
#include "LCD.h"
#include "Timer.h"
#include "Serial.h"
#include "Funcoes.h"

// Variáveis globais externas
extern volatile unsigned char sessao_ativa;
extern volatile char estado_caixa;
extern volatile char piscar_led;
extern volatile char inatividade_segundos;
extern volatile unsigned char sessao_encerrada_por_inatividade;
extern volatile unsigned char aguardando_resposta_saldo;
extern char saldo_recebido[12];
extern volatile signed char saque_aprovado;
extern volatile signed char pagamento_aprovado;
extern volatile char existe;
extern volatile char fora_de_funcionamento;

// Mensagem que indica usuário não autorizado
const char mensagem_esperada[15] = {
	'N', 'a', 'o', ' ', 'a', 'u', 't', 'o', 'r', 'i', 'z', 'a', 'd', 'o'
};

char nomeagencia[32]; // Armazena nome do cliente e número da agência
char nome[30];        // Armazena apenas o nome

// Variáveis auxiliares para controle de comunicação serial
volatile unsigned char contador_operacional = 0;
volatile unsigned char contador = 0;
volatile unsigned char USART_ReceiveBuffer;
volatile unsigned char asci_primeiro_byte = 0;
volatile unsigned char asci_segundo_byte = 0;
volatile unsigned char short_terceiro_byte = 0;
volatile unsigned char short_quarto_byte = 0;
char diameshoramin[4];
volatile char hora_recebida = 0;
volatile char dia = 0;
volatile char hora = 0;
volatile char min = 0;
volatile char seg = 0;

// Interrupção de recepção serial USART0
ISR(USART0_RX_vect) {
	USART_ReceiveBuffer = UDR0; // Lê byte recebido

	// Início da recepção: primeiro byte deve ser 'S'
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

	// Comando de liberar ou travar o caixa
	if (contador == 2) {
		if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'L') {
			caixa_liberado();     // Função para liberar caixa
			estado_caixa = 1;     // Marca caixa como liberado
			contador = 0;
			} else if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'T') {
			caixa_travado();      // Função para travar caixa
			estado_caixa = 0;     // Marca caixa como travado
			lcd_limpar();         // Limpa display
			lcd_string("FORA DE");
			lcd_comando(0xC0);
			lcd_string("OPERACAO");
			contador = 0;
			} else if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'H') {
			char j = 0;
			for (j = 0; j < 4; j++){
				while (!(UCSR0A & (1 << RXC0)));
				diameshoramin[j] = UDR0;
			}
			diameshoramin[j] = '\0';
			dia = diameshoramin[0];
			hora = diameshoramin[2];
			min = diameshoramin[3];
			caixa_data_hora();
			contador = 0;
		}
	}

	// Comando de saque, pagamento, saldo ou entrada
	if (contador == 3) {
		if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'S') {
			if (short_terceiro_byte == 'O') {
				saque_aprovado = 1; // Saque autorizado
				} else if (short_terceiro_byte == 'I') {
				saque_aprovado = 0; // Saque negado (saldo insuficiente)
			}
			contador = 0;
			} else if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'P') {
			if (short_terceiro_byte == 'O') {
				pagamento_aprovado = 1; // Pagamento autorizado
				} else if (short_terceiro_byte == 'I') {
				pagamento_aprovado = 0; // Pagamento negado
			}
			contador = 0;
			} else if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'V') {
			// Resposta de saldo
			unsigned char tamanho = short_terceiro_byte;
			char i;
			if (tamanho > 11) {
				tamanho = 11;
			}
			for (i = 0; i < tamanho; i++) {
				while (!(UCSR0A & (1 << RXC0)))
				; // Espera recepção
				saldo_recebido[i] = UDR0; // Lê caractere do saldo
			}
			saldo_recebido[i] = '\0'; // Finaliza string
			aguardando_resposta_saldo = 0;
			contador = 0;
			} else if (asci_primeiro_byte == 'S' && asci_segundo_byte == 'E') {
			// Resposta de entrada de cliente
			char n = short_terceiro_byte;
			char iguais = 0;
			char j = 0;
			for (j = 0; j < n; j++) {
				while (!(UCSR0A & (1 << RXC0)))
				; // Espera recepção
				nomeagencia[j] = UDR0; // Lê nome+agência
			}
			nomeagencia[j] = '\0';

			// Verifica se resposta foi "Nao autorizado"
			for (j = 0; j < n; j++) {
				if (nomeagencia[j] == mensagem_esperada[j]) {
					iguais++;
				}
			}

			if (iguais == n) {
				existe = 2; // Não autorizado
				} else {
				existe = 1; // Autorizado
				for (j = 0; j < (n - 3); j++) {
					nome[j] = nomeagencia[j]; // Extrai apenas nome
				}
			}
			contador = 0;
			} else {
			contador = 0; // Reset em caso de comando inválido
		}
	}
}

// Interrupção de Timer1 - usado para controlar inatividade e status
ISR(TIMER1_COMPA_vect) {
	
	if (estado_caixa == 1 && fora_de_funcionamento == 0){
	inatividade_segundos++; // Incrementa tempo de inatividade
	}

	// Após 18s, começa a piscar LED
	if (inatividade_segundos >= 18 && !piscar_led) {
		piscar_led = 1;
		timer3_ctc_init(); // Inicia piscar LED
	}

	// Após 30s, encerra sessão por inatividade
	if (inatividade_segundos >= 30 ) {
		timer3_stop(); // Para o LED
		inatividade_segundos = 0;
		piscar_led = 0;
		sessao_ativa = 0;
		sessao_encerrada_por_inatividade = 1;
	}

	// A cada 30s, envia status de funcionamento do caixa
	if (estado_caixa == 1) {
		contador_operacional++;
		if (contador_operacional >= 30) {
			caixa_operando_normalmente(); // Informa que caixa está ok
			contador_operacional = 0;
		}
	}
	
	// Relógio interno do caixa
	if (hora_recebida == 1) {
		seg++;
		if (seg >= 60) {
			seg = 0;
			min++;
			if (min >= 60) {
				min = 0;
				hora++;
				if (hora >= 24) {
					hora = 0;
					dia++;
				}
			}
		}
		if (hora >= 8 && hora < 20) {
			fora_de_funcionamento = 0;  // Dentro do horário
		}
		else {
			fora_de_funcionamento = 1;  // Fora de horário
		}
	}
}

// Interrupção de Timer3 - usado para piscar LED em 2Hz
ISR(TIMER3_COMPA_vect) {
	PORTA ^= (1 << PA0); // Alterna estado do pino PA0 (LED)
}
