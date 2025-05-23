#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Serial.h"
#include "Timer.h"

extern volatile char estado_caixa; // 0 = travado, 1 = liberado
extern volatile char bloqueado;
extern volatile char liberado;

// Inicializa a comunicação serial (USART0)
void USART_Init() {
	UBRR0H = 0;
	UBRR0L = 51; // Baud rate 19200 para 16MHz

	UCSR0C = 0x26; // Assíncrono, paridade par, 1 stop, 8 bits
	UCSR0B = 0x98; // RX, TX e RX interrupt habilitados
	sei();
}

// Envia um caractere (modo bloqueante)
char USART_Transmit(unsigned char dado) {
	while (!(UCSR0A & (1 << UDRE0)));
	UDR0 = dado;
	return dado;
}

// CAIXA → SERVIDOR

void caixa_liberado() {
	USART_Transmit('C');
	USART_Transmit('L');
	delay_10ms();
}

void caixa_travado() {
	USART_Transmit('C');
	USART_Transmit('T');
	delay_10ms();
}

void caixa_data_hora() {
	USART_Transmit('C');
	USART_Transmit('H');
	delay_10ms();
}

void caixa_entrada_cliente(char* usuario, char* senha) {
	USART_Transmit('C');
	USART_Transmit('E');

	for (uint8_t i = 0; i < 6; i++) USART_Transmit(usuario[i]);
	for (uint8_t i = 0; i < 6; i++) USART_Transmit(senha[i]);

	delay_10ms();
}

void caixa_saldo() {
	USART_Transmit('C');
	USART_Transmit('V');
	delay_10ms();
}

void caixa_saque(char* valor_saque) {
	uint8_t i = 0;
	uint8_t n = 0;

	while (valor_saque[n] != '\0' && n < 127) n++;

	USART_Transmit('C');
	USART_Transmit('S');
	USART_Transmit(n);

	for (i = 0; i < n; i++) USART_Transmit(valor_saque[i]);

	delay_10ms();
}

void caixa_pagamento(char* banco_convenio_valor) {
	uint8_t i = 0;
	uint8_t n = 0;

	while (banco_convenio_valor[n] != '\0' && n < 127) n++;

	USART_Transmit('C');
	USART_Transmit('P');
	USART_Transmit(n);

	for (i = 0; i < n; i++) USART_Transmit(banco_convenio_valor[i]);

	delay_10ms();
}

void caixa_boleto_recebido() {
	USART_Transmit('C');
	USART_Transmit('B');
	delay_10ms();
}

void imprime_comprovante() {
	// Implementar: decide se LCD, serial, ou outro meio
	USART_Transmit('C');
	USART_Transmit('R'); // Exemplo de comando de recibo
	delay_10ms();
}

void sessao_finalizada() {
	USART_Transmit('C');
	USART_Transmit('F');
	delay_10ms();
}

void caixa_operando_normalmente() {
	USART_Transmit('C');
	USART_Transmit('O');
	delay_10ms();
}

// OPÇÃO A – Solicita nova senha silábica para o usuário
void caixa_troca_senha(char* usuario) {
	USART_Transmit('C');
	USART_Transmit('M');

	for (uint8_t i = 0; i < 6; i++) USART_Transmit(usuario[i]);

	delay_10ms();
}
