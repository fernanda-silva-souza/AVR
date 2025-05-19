#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Teclado.h"
#include "LCD.h"
#include "Serial.h"

// Incluir demais funções .h aqui depois

// Inicializa a USART
void USART_Init() {
	UBRR0H = 0; // Parte alta do registrador de baud rate (19200)
	UBRR0L = 51; // Parte baixa

	UCSR0C = 0x26; // Formato de comunicação | Modo assíncrono | Paridade par | 1 stop bit | 8 bits de dados
	UCSR0B = 0x98; // Habilita recepção, transmissão e interrupção de recepção
}


// Envia um caractere da USART (modo bloqueante)
void USART_Transmit(unsigned char dado) {
	while (!(UCSR0A & (1 << 6))); // Espera um caractere ser recebido
	UDR0 = dado; // Retorna o caractere recebido
}

//Envia resposta ao servidor: Caixa está liberado
void caixa_liberado() {
	USART_Transmit('C');
	USART_Transmit('L');
	estado_caixa = 1;
	_delay_ms(300);
}

//Envia resposta ao servidor: Caixa está travado
void caixa_travado() {
	USART_Transmit('C');
	USART_Transmit('T');
	estado_caixa = 0;
	_delay_ms(300);
}

//Envia resposta ao servidor: Caixa recebeu data e hora
void caixa_data_hora() {
	USART_Transmit('C');
	USART_Transmit('H');
	_delay_ms(300);
}

//Envio de entrada de cliente
void caixa_entrada_cliente(char* usuario, char* senha) {
	char i = 0;

	USART_Transmit('C');
	USART_Transmit('E');
	for (i = 0; i < 6; i++) {  // Envia os 6 caracteres do usuário
		USART_Transmit(usuario[i]);
	}
	for (uint8_t i = 0; i < 6; i++) { // Envia os 6 caracteres da senha
		USART_Transmit(senha[i]);
	}
	_delay_ms(300);
}

//Envio de solicitação para verificação de saldo
void caixa_saldo() {
	USART_Transmit('C');
	USART_Transmit('V');
	_delay_ms(300);
}

//Envio de solicitação de saque
void caixa_saque(char* valor_saque) {
	char n = 0;
	char i = 0;

	// Conta quantos caracteres tem na string 'valor_saque'
	while (valor_saque[n] != '\0' && n < 127) {
		n++;
	}

	USART_Transmit('C');
	USART_Transmit('S');
	USART_Transmit(n);

	// Envia os caracteres do valor
	for (char i = 0; i < n; i++) {
		USART_Transmit(valor_saque[i]);
	}
	_delay_ms(300);
}

//Envia resposta ao servidor: Boleto recebido
void caixa_boleto_recebido() {
	USART_Transmit('C');
	USART_Transmit('B');
	_delay_ms(300);
}

//Envio de solicitação de pagamento
void caixa_pagamento(char* banco_convenio_valor) {
	char n = 0;
	char i = 0;

	// Conta quantos caracteres tem na string 'valor_pagamento'
	while (banco_convenio_valor[n] != '\0' && n < 127) {
		n++;
	}

	USART_Transmit('C');
	USART_Transmit('P');
	USART_Transmit(n);

	// Envia os caracteres de banco_convenio_valor
	for (char i = 0; i < n; i++) {
		USART_Transmit(banco_convenio_valor[i]);
	}
	_delay_ms(300);
}

//Envio de solicitação de impressão de comprovante
void imprime_comprovante() {
	// Pensar sobre como será feito	
}

//Envio de finalização de sessão
void sessao_finalizada() {
	USART_Transmit('C');
	USART_Transmit('F');
	_delay_ms(300);
}

//Envio de confirmação de estado operacional (deve ser enviado periodicamente)
void caixa_operando_normalmente() {
	USART_Transmit('C');
	USART_Transmit('O');
	_delay_ms(300);
}


// IMPLEMENTAÇÃO OPÇÃO A
//Envio de solicitação de troca de senha
void caixa_troca_senha(char* usuario) {
	char i = 0;

	USART_Transmit('C');
	USART_Transmit('M');
	for (i = 0; i < 6; i++) {  // Envia os 6 caracteres do usuário
		USART_Transmit(usuario[i]);
	}
	_delay_ms(300);
}
