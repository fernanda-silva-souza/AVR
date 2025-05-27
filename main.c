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

// Variáveis de controle global
volatile signed char saque_aprovado = -1; // -1: aguardando, 1: sucesso, 0: insuficiente
volatile signed char pagamento_aprovado = -1;
volatile uint8_t sessao_ativa = 0;
volatile char estado_caixa = 0;
volatile char piscar_led = 0;
volatile char inatividade_segundos = 0;
volatile uint8_t sessao_encerrada_por_inatividade = 0;
volatile uint8_t aguardando_resposta_saldo = 0;
char saldo_recebido[12] = ""; // para armazenar até 11 dígitos + '\0'
volatile char existe = 0;


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
	lcd_comando(0xC0);
	lcd_string("Senha bloqueada");
	_delay_ms(3000);
	tela_bem_vindo();
}

// === MENU DE OPERAÇÕES ===

void menu_operacoes() {
	char opcao = 0;

	while (1) {
		
		if (estado_caixa == 0) return;
		if (sessao_ativa == 0) return;
		
		lcd_limpar();
		lcd_string("1-Saque 2-Saldo");
		lcd_comando(0xC0);
		lcd_string("3-Pagamento");

		opcao = le_tecla();

		if (opcao == '1') {
			lcd_limpar();
			lcd_string("Valor do saque:");
			lcd_comando(0xC0);
			
			char valor[10] = "";
			uint8_t pos = 0;

			while (1) {
				char tecla = le_tecla();

				if (tecla == '#') break;
				if (tecla == '*' && pos > 0) {
					pos--;
					lcd_comando(0xC0 + pos);
					lcd_dado(' ');
					lcd_comando(0xC0 + pos);
				}
				else if (tecla >= '0' && tecla <= '9' && pos < 6) {
					valor[pos++] = tecla;
					lcd_dado(tecla);
				}
			}
			valor[pos] = '\0';

			// Envia valor ao servidor
			saque_aprovado = -1;
			caixa_saque(valor);

			// Aguarda resposta do servidor
			while (saque_aprovado == -1);

			// Exibe resultado
			lcd_limpar();
			if (saque_aprovado == 1) {
				lcd_string("Saque efetuado");
				} else if (saque_aprovado == 0) {
				lcd_string("Saldo");
				lcd_comando(0xC0);
				lcd_string("insuficiente");
			}
			_delay_ms(3000); // Aumente o delay para ter tempo de ler o valor
		}


		else if (opcao == '2') {
			// Solicita saldo ao servidor
			aguardando_resposta_saldo = 1;
			caixa_saldo();

			// Espera a resposta do servidor
			while (aguardando_resposta_saldo);

			// Formata o valor recebido
			uint8_t len = strlen(saldo_recebido);
			char saldo_formatado[16] = "";
			uint8_t i, j = 0;

			if (len <= 2) {
				// Exemplo: "00", "50"
				sprintf(saldo_formatado, "R$0,%s", saldo_recebido);
				} else {
				for (i = 0; i < len - 2; i++) {
					saldo_formatado[j++] = saldo_recebido[i];
				}
				saldo_formatado[j++] = ',';
				saldo_formatado[j++] = saldo_recebido[len - 2];
				saldo_formatado[j++] = saldo_recebido[len - 1];
				saldo_formatado[j] = '\0';

				// Adiciona "R$" na frente
				char final[18];
				sprintf(final, "R$%s", saldo_formatado);
				strcpy(saldo_formatado, final);
			}

			// Mostra no LCD
			lcd_limpar();
			lcd_string("Saldo atual:");
			lcd_comando(0xC0);
			lcd_string(saldo_formatado);
			_delay_ms(3000); // Mostra por 3s antes de voltar ao menu
		}


		else if (opcao == '3') {
			lcd_limpar();
			lcd_string("Cod. do boleto:");

			char codigo_boleto_completo[48];
			uint8_t total_pos_boleto = 0;
			uint8_t digitos_segmento_alvo[3] = {16, 16, 15}; // Digitos de cada parte

			for(uint8_t i=0; i<48; i++) codigo_boleto_completo[i] = '\0';

			for (uint8_t segmento_idx = 0; segmento_idx < 3; segmento_idx++) {
				lcd_comando(0xC0);
				for (uint8_t k = 0; k < 16; k++) lcd_dado(' ');
				lcd_comando(0xC0);

				char buffer_segmento_atual[17] = "";
				uint8_t pos_no_segmento = 0;
				uint8_t max_digitos_segmento = digitos_segmento_alvo[segmento_idx];

				while (1) {
					char tecla = le_tecla();

					if (tecla == '#') { 
						if (pos_no_segmento == max_digitos_segmento) {
							strncpy(codigo_boleto_completo + total_pos_boleto, buffer_segmento_atual, pos_no_segmento);
							total_pos_boleto += pos_no_segmento;
							break;
							} else {
							// Not enough digits, '#' is ignored. User must complete the segment.
						}
						} else if (tecla == '*') { // Backspace
						if (pos_no_segmento > 0) {
							pos_no_segmento--;
							buffer_segmento_atual[pos_no_segmento] = '\0';
							lcd_comando(0xC0 + pos_no_segmento); // Move cursor back
							lcd_dado(' ');                   // Erase char on LCD
							lcd_comando(0xC0 + pos_no_segmento); // Move cursor back again
						}
						} else if (tecla >= '0' && tecla <= '9') { // Numeric input
						if (pos_no_segmento < max_digitos_segmento) {
							if (pos_no_segmento < 16) { // Ensure it fits on one LCD line display
								buffer_segmento_atual[pos_no_segmento] = tecla;
								lcd_dado(tecla); // Display digit
								pos_no_segmento++;
							}
						}
					}
					// Allow session timeout to break this loop if necessary via main loop logic
					if (estado_caixa == 0 || sessao_ativa == 0) return; // Exit if session ends
				}
			}
			codigo_boleto_completo[total_pos_boleto] = '\0'; // Ensure null termination

			// Proceed only if the full 47 digits were collected
			if (total_pos_boleto == 47) {
				char banco_str[4];
				char convenio_str[5];
				char valor_ultimos_10_digitos_str[11];
				char valor_final_para_envio_str[12]; // For value like "2700" or "123456"

				// 1. Extrair Banco (primeiros 3 dígitos)
				strncpy(banco_str, codigo_boleto_completo, 3);
				banco_str[3] = '\0';

				// 2. Extrair Convênio (dígitos 5, 6, 7, 8 -- 0-indexed: 4, 5, 6, 7)
				strncpy(convenio_str, codigo_boleto_completo + 4, 4);
				convenio_str[4] = '\0';

				// 3. Extrair Valor (últimos 10 dígitos)
				strncpy(valor_ultimos_10_digitos_str, codigo_boleto_completo + 37, 10);
				valor_ultimos_10_digitos_str[10] = '\0';
				
				// Convert 10-digit value string (e.g., "0000002700") to integer, then to string (e.g., "2700")
				long valor_long = atol(valor_ultimos_10_digitos_str); // stdlib.h needed
				sprintf(valor_final_para_envio_str, "%ld", valor_long); // stdio.h needed

				// Enviar para o servidor
				pagamento_aprovado = -1; // Reset status before sending
				caixa_pagamento(banco_str, convenio_str, valor_final_para_envio_str);
				
				while (pagamento_aprovado == -1);
				
				lcd_limpar();
				if (pagamento_aprovado == 1) { // SPO - OK
					lcd_string("Pagamento");
					lcd_comando(0xC0);
					lcd_string("efetuado");
					} else if (pagamento_aprovado == 0) { // SPI - Saldo Insuficiente
					lcd_string("Saldo");
					lcd_comando(0xC0);
					lcd_string("Insuficiente");
					} 
				}
				_delay_ms(3000); // Mostrar mensagem por 3 segundos
			}
		_delay_ms(1000); // Volta ao menu após operação
	}
}


// === MAIN ===

int main() {
	USART_Init();
	DDRK = 0x0F;
	PORTK = 0xFF;
	DDRA |= (1 << 0);
	inicializa_lcd();
	timer1_ctc_init();
	sei();

	while (1) {
		// 1. Se o terminal ainda não foi liberado pelo servidor
		if (estado_caixa == 0) {
			lcd_limpar();
			lcd_string("FORA DE");
			lcd_comando(0xC0);
			lcd_string("OPERACAO");

			while (estado_caixa == 0);  // aguarda o servidor liberar
			_delay_ms(300);
			tela_bem_vindo();
		}

		// 2. Se a sessão foi encerrada por inatividade
		if (sessao_encerrada_por_inatividade) {
			lcd_limpar();
			lcd_string("Sessao");
			lcd_comando(0xC0);
			lcd_string("encerrada");

			sessao_encerrada_por_inatividade = 0;
			sessao_finalizada();
			PORTA &= ~(1 << PA0);  // Desliga LED ao encerrar sessão

			while (le_tecla() != '#');
			tela_bem_vindo();
		}

		// 3. Se o servidor liberou e cliente ainda não autenticou
		if (estado_caixa == 1 && sessao_ativa == 0) {
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
				// Envia dados ao servidor apenas para registro
				caixa_entrada_cliente(usuario, senha);
				if (existe == 1) {
					sessao_ativa = 1;
					menu_operacoes();
					tela_bem_vindo();
					} else if (existe == 2) {
					if (tentativas >= 2) {
						acesso_negado();
						} else {
						senha_invalida();
					}
				}
				existe = 0;
			}
		}
	}
}
