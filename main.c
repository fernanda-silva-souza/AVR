#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "Teclado.h"
#include "LCD.h"
#include "Serial.h"
#include "Timer.h"
#include "Funcoes.h"

// Declaração de variáveis globais
volatile signed char saque_aprovado = -1; // -1: não iniciado, 0: negado, 1: aprovado
volatile signed char pagamento_aprovado = -1; // -1: não iniciado, 0: negado, 1: aprovado
volatile unsigned char sessao_ativa = 0; // Flag de sessão ativa
volatile char estado_caixa = 0; // 0: travado, 1: liberado
volatile char piscar_led = 0; // Flag para controle do piscar do LED
volatile char inatividade_segundos = 0; // Contador de inatividade da sessão
volatile unsigned char sessao_encerrada_por_inatividade = 0; // Flag de encerramento automático
volatile unsigned char aguardando_resposta_saldo = 0; // Flag de espera por resposta do servidor
char saldo_recebido[12] = ""; // Armazena o saldo recebido do servidor
volatile char existe = 0; // 0: ainda não verificado, 1: cliente válido, 2: não autorizado
extern char nome[30]; // Nome do cliente (externo)
char ultimo_usuario[30]; // Armazena último usuário
char tipo_transacao[20]; // Tipo de transação realizada
char detalhes_transacao[128]; // Detalhes da transação realizada
char usuario[7] = ""; // Armazena os 6 dígitos do usuário
char senha[7] = ""; // Armazena os 6 dígitos da senha
char senha_padrao[7] = "123456"; // Senha padrão (não usada na lógica atual)
unsigned char tentativas = 0; // Contador de tentativas incorretas
unsigned char usuario_confirmado = 0; // Flag de usuário já digitado e confirmado
unsigned char pos_usuario = 0; // Posição de digitação do usuário
unsigned char pos_senha = 0; // Posição de digitação da senha
volatile char fora_de_funcionamento = 0; // Relógio interno do caixa

// === MAIN ===
int main() {
	USART_Init(); // Inicializa comunicação serial
	DDRK = 0x0F; // Define os 4 bits menos significativos do PORTK como saída
	PORTK = 0xFF; // Ativa pull-ups ou inicializa como alto
	DDRA |= (1 << 0); // Define PA0 como saída (LED)
	inicializa_lcd(); // Inicializa o display LCD
	timer1_ctc_init(); // Inicializa Timer1 em modo CTC
	sei(); // Habilita interrupções globais

	while (1) {
		// 1. Se o terminal ainda não foi liberado pelo servidor
		if (estado_caixa == 0 || fora_de_funcionamento == 1) {
			lcd_limpar(); // Limpa o LCD
			lcd_string("FORA DE"); // Primeira linha
			lcd_comando(0xC0); // Vai para segunda linha
			lcd_string("OPERACAO"); // Segunda linha

			while (estado_caixa == 0 || fora_de_funcionamento == 1);  // Aguarda o servidor liberar
			_delay_ms(10);
			tela_bem_vindo(); // Exibe tela inicial
		}

		// 2. Se a sessão foi encerrada por inatividade
		if (sessao_encerrada_por_inatividade) {
			lcd_limpar(); // Limpa o LCD
			lcd_string("Sessao"); // Primeira linha
			lcd_comando(0xC0); // Segunda linha
			lcd_string("encerrada"); // Mensagem de sessão encerrada

			sessao_encerrada_por_inatividade = 0; // Reseta flag
			sessao_finalizada(); // Função de encerramento
			PORTA &= ~(1 << PA0);  // Desliga LED ao encerrar sessão

			while (le_tecla() != '#'); // Aguarda usuário apertar #
			tela_bem_vindo(); // Exibe tela de boas-vindas
		}

		// 3. Se o servidor liberou e cliente ainda não autenticou
		if (estado_caixa == 1 && sessao_ativa == 0) {
			char tecla = le_tecla(); // Lê tecla pressionada

			if (tecla == '*') {
				if (!usuario_confirmado) tela_bem_vindo(); // Volta para tela de usuário
				else tela_senha(); // Vai para tela de senha
				} else if (!usuario_confirmado && pos_usuario < 6 && tecla >= '0' && tecla <= '9') {
				usuario[pos_usuario++] = tecla; // Armazena dígito do usuário
				lcd_dado(tecla); // Mostra no LCD
				} else if (tecla == '#' && pos_usuario == 6 && !usuario_confirmado) {
				usuario_confirmado = 1; // Usuário confirmado
				tela_senha(); // Vai para tela de senha
				} else if (usuario_confirmado && pos_senha < 6 && tecla >= '0' && tecla <= '9') {
				senha[pos_senha++] = tecla; // Armazena dígito da senha
				lcd_dado('*'); // Mostra * no LCD
				} else if (tecla == '#' && usuario_confirmado && pos_senha == 6) {
				// Envia dados ao servidor apenas para registro
				caixa_entrada_cliente(usuario, senha); // Envia usuário e senha ao servidor

				if (existe == 1) {
					sessao_ativa = 1; // Sessão liberada
					menu_operacoes(); // Vai para menu
					tela_bem_vindo(); // Ao sair, volta para tela inicial
					} else if (existe == 2) {
					if (tentativas >= 2) {
						acesso_negado(); // Bloqueia após 3 tentativas
						} else {
						senha_invalida(); // Mostra senha inválida
					}
				}
				existe = 0; // Reseta flag
			}
		}
	}
}
