#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "Teclado.h"
#include "LCD.h"
#include "Serial.h"
#include "Timer.h"
#include "Funcoes.h"

// Variáveis de controle global
volatile char bloqueado = 0;
volatile char liberado = 0;
volatile char piscar_led = 0;
volatile char inatividade_segundos = 0;

// Variáveis de autenticação
char usuario[7] = "";
char senha[7] = "";
char senha_padrao[7] = "123456";
uint8_t tentativas = 0;
uint8_t usuario_confirmado = 0;
uint8_t pos_usuario = 0;
uint8_t pos_senha = 0;

// === TELAS ===

void tela_bem_vindo() {
	lcd_limpar();
	lcd_comando(0x80);
	lcd_string("BanrisUFRGS");
	lcd_comando(0xC0);
	lcd_string("Usuario: ");
	pos_usuario = 0;
	usuario_confirmado = 0;
	tentativas = 0;
	for (uint8_t i = 0; i < 6; i++) {
		usuario[i] = '\0';
		senha[i] = '\0';
	}
}

void tela_senha() {
	lcd_comando(0xC0);
	lcd_string("Senha:          ");
	lcd_comando(0xC0 + 7);
	pos_senha = 0;
	for (uint8_t i = 0; i < 6; i++) senha[i] = '\0';
}

void login_completo() {
	lcd_limpar();
	lcd_string("Login completo");
	_delay_ms(1000);
}

void senha_invalida() {
	tentativas++;
	lcd_comando(0xC0);
	lcd_string("Senha invalida");
	_delay_ms(1500);
	tela_senha();
}

void acesso_negado() {
	lcd_limpar();
	lcd_string("Acesso negado");
	liberado = 0;
	while (1); // trava terminal
}

// === MENU DE OPERAÇÕES ===

void menu_operacoes() {
	char opcao = 0;
	char valor[10] = "";
	uint8_t pos = 0;

	while (1) {
		lcd_limpar();
		lcd_string("1-Saque 2-Saldo");
		lcd_comando(0xC0);
		lcd_string("3-Pagamento");

		opcao = le_tecla();

		// Aguarda confirmação com '#'
		while (le_tecla() != '#');

		if (opcao == '1') {
			lcd_limpar();
			lcd_string("Valor: ");
			pos = 0;

			while (1) {
				char tecla = le_tecla();

				if (tecla == '#') break;
				if (tecla == '*' && pos > 0) {
					pos--;
					lcd_comando(0xC0 + 7 + pos);
					lcd_dado(' ');
					lcd_comando(0xC0 + 7 + pos);
				}
				else if (tecla >= '0' && tecla <= '9' && pos < 6) {
					valor[pos++] = tecla;
					lcd_dado(tecla);
				}
			}

			valor[pos] = '\0';
			caixa_saque(valor);
		}

		else if (opcao == '2') {
			lcd_limpar();
			lcd_string("Consultando...");
			caixa_saldo();
			_delay_ms(1000);
		}

		else if (opcao == '3') {
			lcd_limpar();
			lcd_string("Cod. Banco:");
			lcd_comando(0xC0);
			pos = 0;

			while (1) {
				char tecla = le_tecla();

				if (tecla == '#') break;
				if (tecla == '*' && pos > 0) {
					pos--;
					lcd_comando(0xC0 + pos);
					lcd_dado(' ');
					lcd_comando(0xC0 + pos);
				}
				else if (tecla >= '0' && tecla <= '9' && pos < 12) {
					valor[pos++] = tecla;
					lcd_dado(tecla);
				}
			}

			valor[pos] = '\0';
			caixa_pagamento(valor);
		}

		_delay_ms(1000); // Aguarda antes de voltar ao menu
	}
}

// === MAIN ===

int main() {
	USART_Init();
	DDRK = 0x0F;     // Configura linhas do teclado
	PORTK = 0xFF;    // Pull-ups ativados

	inicializa_lcd();
	timer1_ctc_init(); // Timer de inatividade
	sei();              // Habilita interrupções globais

	tela_bem_vindo();

	while (1) {
		if (liberado == 1) {
			caixa_liberado();

			char tecla = le_tecla();

			if (tecla == '*') {
				if (!usuario_confirmado) tela_bem_vindo();
				else tela_senha();
			}
			else if (!usuario_confirmado && pos_usuario < 6 && tecla >= '0' && tecla <= '9') {
				usuario[pos_usuario++] = tecla;
				lcd_dado(tecla);
			}
			else if (tecla == '#' && pos_usuario == 6 && !usuario_confirmado) {
				usuario_confirmado = 1;
				tela_senha();
			}
			else if (usuario_confirmado && pos_senha < 6 && tecla >= '0' && tecla <= '9') {
				senha[pos_senha++] = tecla;
				lcd_dado('*');
			}
			else if (tecla == '#' && usuario_confirmado && pos_senha == 6) {
				uint8_t match = 1;
				for (uint8_t i = 0; i < 6; i++) {
					if (usuario[i] != senha[i]) {
						match = 0;
						break;
					}
				}

				// Tenta senha padrão se a digitada não bater com o usuário
				if (!match) {
					match = 1;
					for (uint8_t i = 0; i < 6; i++) {
						if (senha[i] != senha_padrao[i]) {
							match = 0;
							break;
						}
					}
				}

				if (match) {
					login_completo();
					menu_operacoes();   // Acesso às opções
					tela_bem_vindo();   // Volta à tela inicial após uso
					} else {
					if (tentativas >= 2) {
						acesso_negado();
						} else {
						senha_invalida();
					}
				}
			}
		}
	}
}
