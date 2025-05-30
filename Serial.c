#define F_CPU 16000000UL // Define a frequência do clock para 16 MHz
#include <avr/io.h> // Biblioteca para manipulação de registradores de I/O
#include <avr/interrupt.h> // Biblioteca para uso de interrupções
#include <util/delay.h> // Biblioteca para uso da função _delay_ms()
#include "Serial.h" // Arquivo de cabeçalho com as declarações da serial
#include "Timer.h" // Arquivo de cabeçalho com as declarações do timer

// Variáveis globais externas utilizadas em outros arquivos
extern volatile char estado_caixa; // 0 = travado, 1 = liberado
extern volatile char bloqueado; // Flag para estado bloqueado
extern volatile char liberado; // Flag para estado liberado
extern volatile char hora_recebida;

// === Inicializa a comunicação serial (USART0) ===
void USART_Init() {
	UBRR0H = 0; // Parte alta do registrador de baud rate
	UBRR0L = 51; // Baud rate 19200 para clock de 16MHz

	UCSR0C = 0x26; // Configura: Assíncrono, paridade par, 1 stop bit, 8 bits de dados
	UCSR0B = 0x98; // Habilita recepção, transmissão e interrupção por recepção
	sei(); // Habilita interrupções globais
}

// === Envia um caractere via USART (modo bloqueante) ===
char USART_Transmit(unsigned char dado) {
	while (!(UCSR0A & (1 << UDRE0))); // Espera o registrador estar pronto para enviar
	UDR0 = dado; // Coloca o dado no registrador de transmissão
	return dado; // Retorna o caractere enviado
}

// === Funções de comunicação: CAIXA → SERVIDOR ===

// Informa ao servidor que o caixa foi liberado
void caixa_liberado(void) {
	USART_Transmit('C');
	USART_Transmit('L');
	_delay_ms(10);
}

// Informa ao servidor que o caixa foi travado
void caixa_travado(void) {
	USART_Transmit('C');
	USART_Transmit('T');
	_delay_ms(10);
}

// Solicita data e hora ao servidor
void caixa_data_hora(void) {
	USART_Transmit('C');
	USART_Transmit('H');
	hora_recebida = 1;
	_delay_ms(10);
}

// Envia usuário e senha digitados para o servidor
void caixa_entrada_cliente(char* usuario, char* senha) {
	USART_Transmit('C');
	USART_Transmit('E');

	for (char i = 0; i < 6; i++) USART_Transmit(usuario[i]); // Envia 6 dígitos do usuário
	for (char i = 0; i < 6; i++) USART_Transmit(senha[i]);   // Envia 6 dígitos da senha

	_delay_ms(10);
}

// Solicita saldo ao servidor
void caixa_saldo(void) {
	USART_Transmit('C');
	USART_Transmit('V');
	_delay_ms(10);
}

// Envia pedido de saque ao servidor com o valor digitado
void caixa_saque(char* valor_saque) {
	unsigned char i = 0;
	unsigned char n = 0;

	// Conta o número de caracteres no valor do saque
	while (valor_saque[n] != '\0' && n < 127) n++;

	USART_Transmit('C');
	USART_Transmit('S');
	USART_Transmit(n); // Envia o tamanho da string

	for (i = 0; i < n; i++) USART_Transmit(valor_saque[i]); // Envia os dígitos do valor

	_delay_ms(10);
}

// Envia os dados para pagamento de boleto (banco + convênio + valor)
void caixa_pagamento(char* banco, char* convenio, char* valor_str) {
	unsigned char i = 0;
	unsigned char len_banco = 0;
	unsigned char len_convenio = 0;
	unsigned char len_valor = 0;

	// Calcula o comprimento de cada campo
	while (banco[len_banco] != '\0' && len_banco < 255) len_banco++;
	while (convenio[len_convenio] != '\0' && len_convenio < 255) len_convenio++;
	while (valor_str[len_valor] != '\0' && len_valor < 255) len_valor++;
	
	// n é o total de bytes no payload (banco + convenio + valor)
	unsigned char n_bytes = len_banco + len_convenio + len_valor;

	USART_Transmit('C');
	USART_Transmit('P');
	USART_Transmit(n_bytes); // Envia o tamanho total da mensagem

	for (i = 0; i < len_banco; i++) USART_Transmit(banco[i]); // Envia banco
	for (i = 0; i < len_convenio; i++) USART_Transmit(convenio[i]); // Envia convênio
	for (i = 0; i < len_valor; i++) USART_Transmit(valor_str[i]); // Envia valor

	_delay_ms(10);
}

// Confirma ao servidor que o boleto foi recebido com sucesso
void caixa_boleto_recebido(void) {
	USART_Transmit('C');
	USART_Transmit('B');
	_delay_ms(10);
}

// Envia dados do comprovante para o servidor (para possível impressão)
void imprime_comprovante(unsigned char* dados_comprovante) {
	unsigned char data_len = 0;

	// Conta o número de caracteres do comprovante
	while (dados_comprovante[data_len] != '\0' && data_len < 65530) {
		data_len++;
	}

	USART_Transmit('C');
	USART_Transmit('I');

	// Envia o tamanho em dois bytes (big endian)
	USART_Transmit((unsigned char)(data_len >> 8));
	USART_Transmit((unsigned char)(data_len & 0xFF));

	for (unsigned char i = 0; i < data_len; i++) {
		USART_Transmit(dados_comprovante[i]); // Envia os dados
	}
	_delay_ms(10);
}

// Informa ao servidor que a sessão do cliente foi finalizada
void sessao_finalizada(void) {
	USART_Transmit('C');
	USART_Transmit('F');
	_delay_ms(10);
}

// Informa ao servidor que o caixa está funcionando normalmente
void caixa_operando_normalmente(void) {
	USART_Transmit('C');
	USART_Transmit('O');
	_delay_ms(10);
}

// OPÇÃO A – Solicita nova senha silábica para o usuário informado
void caixa_troca_senha(char* usuario) {
	USART_Transmit('C');
	USART_Transmit('M');

	for (unsigned char i = 0; i < 6; i++) USART_Transmit(usuario[i]); // Envia os 6 dígitos do usuário

	_delay_ms(10);
}
