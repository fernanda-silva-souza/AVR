#define F_CPU 16000000UL  // Define a frequência do clock para 16 MHz
#include <avr/io.h>       // Biblioteca de I/O do AVR
#include <avr/interrupt.h> // Biblioteca para interrupções
#include <util/delay.h>
#include <string.h>       // Funções de manipulação de strings
#include <stdio.h>        // Funções de entrada/saída padrão (ex: sprintf)
#include <stdlib.h>       // Para atol()

#include "Teclado.h"      // Módulo para leitura do teclado
#include "LCD.h"          // Módulo para controle do display LCD
#include "Serial.h"       // Módulo para comunicação serial
#include "Timer.h"        // Módulo para controle de tempo/inatividade
#include "Funcoes.h"      // Funções auxiliares do sistema

// Variáveis externas (declaradas em outro arquivo)
extern volatile char inatividade_segundos;
extern volatile char piscar_led;
extern volatile signed char saque_aprovado;
extern volatile signed char pagamento_aprovado;
extern volatile unsigned char sessao_ativa;
extern volatile char estado_caixa;
extern volatile unsigned char aguardando_resposta_saldo;
extern char saldo_recebido[12];
extern char nome[30];
extern char ultimo_usuario[30];
extern char tipo_transacao[20];
extern char detalhes_transacao[128];
extern char usuario[7];
extern char senha[7];
extern unsigned char tentativas;
extern unsigned char usuario_confirmado;
extern unsigned char pos_usuario;
extern unsigned char pos_senha;
extern volatile char fora_de_funcionamento;

// === TELAS ===

// Tela inicial de boas-vindas e entrada de usuário
void tela_bem_vindo(void) {
	lcd_limpar();
	lcd_comando(0x80);
	lcd_string("BanrisUFRGS");
	lcd_comando(0xC0);
	lcd_string("Usuario: ");
	pos_usuario = 0;
	usuario_confirmado = 0;
	tentativas = 0;
	for (unsigned char i = 0; i < 6; i++) {
		usuario[i] = '\0';
		senha[i] = '\0';
	}
}

// Tela de entrada de senha
void tela_senha(void) {
	lcd_comando(0xC0);
	lcd_string("Senha:          ");
	lcd_comando(0xC0 + 7);
	pos_senha = 0;
	for (unsigned char i = 0; i < 6; i++) senha[i] = '\0';
}

// Mensagem de senha inválida e tentativa adicional
void senha_invalida(void) {
	tentativas++;
	lcd_comando(0xC0);
	lcd_string("Senha invalida");
	_delay_ms(3000);
	tela_senha();
}

// Bloqueio de acesso após tentativas inválidas
void acesso_negado(void) {
	lcd_limpar();
	lcd_string("Acesso negado");
	lcd_comando(0xC0);
	lcd_string("Senha bloqueada");
	_delay_ms(3000);
	tela_bem_vindo();
}

// Tela para oferecer impressão de comprovante após uma transação
void oferecer_impressao_comprovante(void) {
	lcd_limpar();
	lcd_string("Imprimir Comp.?");
	lcd_comando(0xC0);
	lcd_string("1-SIM   2-NAO");  // Usar as teclas '1' e '2'

	char escolha = ' ';
	while (escolha != '1' && escolha != '2') {
		escolha = le_tecla();
		if (estado_caixa == 0 || sessao_ativa == 0) return;
	}

	if (escolha == '1') {
		unsigned char data_para_comprovante[200];
		sprintf(data_para_comprovante, "Cliente: %s | Operacao: %s | %s", ultimo_usuario, tipo_transacao, detalhes_transacao);

		imprime_comprovante(data_para_comprovante);

		lcd_limpar();
		lcd_string("Comprovante");
		lcd_comando(0xC0);
		lcd_string("enviado");
		
		_delay_ms(3000);
	}
}

// === MENU DE OPERAÇÕES ===

// Função principal do menu com as opções do caixa
void menu_operacoes(void) {
	char opcao = 0;

	while (1) {

		if (estado_caixa == 0) return;
		if (sessao_ativa == 0) return;
		if (fora_de_funcionamento == 1) return;

		lcd_limpar();
		lcd_string("1-Saque 2-Saldo");
		lcd_comando(0xC0);
		lcd_string("3-Pagamento");

		opcao = le_tecla();

		if (opcao == '1') {  // Saque
			lcd_limpar();
			lcd_string("Valor do saque:");
			lcd_comando(0xC0);

			char valor[10] = "";
			unsigned char pos = 0;

			while (1) {
				char tecla = le_tecla();

				if (tecla == '#') break;  // Confirma valor
				if (tecla == '*' && pos > 0) {  // Apaga último dígito
					pos--;
					lcd_comando(0xC0 + pos);
					lcd_dado(' ');
					lcd_comando(0xC0 + pos);
					} else if (tecla >= '0' && tecla <= '9' && pos < 6) {
					valor[pos++] = tecla;
					lcd_dado(tecla);
				}
			}
			valor[pos] = '\0';

			// Envia o valor ao servidor
			saque_aprovado = -1;
			caixa_saque(valor);

			// Aguarda resposta do servidor
			while (saque_aprovado == -1);

			// Exibe o resultado da operação
			lcd_limpar();
			if (saque_aprovado == 1) {
				lcd_string("Saque efetuado");
				_delay_ms(3000);
				strcpy(ultimo_usuario, nome);
				strcpy(tipo_transacao, "SAQUE");
				long valor_saque_long = atol(valor);
				sprintf(detalhes_transacao, "R$%ld,%02ld", valor_saque_long / 100, valor_saque_long % 100);
				oferecer_impressao_comprovante();
				} else if (saque_aprovado == 0) {
				lcd_string("Saldo");
				lcd_comando(0xC0);
				lcd_string("insuficiente");
				_delay_ms(3000);
			}
		}

		else if (opcao == '2') {  // Consulta de saldo
			aguardando_resposta_saldo = 1;
			caixa_saldo();

			while (aguardando_resposta_saldo);

			unsigned char len = strlen(saldo_recebido);
			char saldo_formatado[16] = "";
			unsigned char i, j = 0;

			if (len <= 2) {
				sprintf(saldo_formatado, "R$0,%s", saldo_recebido);
				} else {
				for (i = 0; i < len - 2; i++) {
					saldo_formatado[j++] = saldo_recebido[i];
				}
				saldo_formatado[j++] = ',';
				saldo_formatado[j++] = saldo_recebido[len - 2];
				saldo_formatado[j++] = saldo_recebido[len - 1];
				saldo_formatado[j] = '\0';

				char final[18];
				sprintf(final, "R$%s", saldo_formatado);
				strcpy(saldo_formatado, final);
			}

			lcd_limpar();
			lcd_string("Saldo atual:");
			lcd_comando(0xC0);
			lcd_string(saldo_formatado);
			_delay_ms(3000);
		}

		else if (opcao == '3') {  // Pagamento de boleto
			lcd_limpar();
			lcd_string("Cod. do boleto:");

			char codigo_boleto_completo[48];
			unsigned char total_pos_boleto = 0;
			unsigned char digitos_segmento_alvo[3] = { 16, 16, 15 };

			for (char i = 0; i < 48; i++) codigo_boleto_completo[i] = '\0';

			for (char segmento_idx = 0; segmento_idx < 3; segmento_idx++) {
				lcd_comando(0xC0);
				for (char k = 0; k < 16; k++) lcd_dado(' ');
				lcd_comando(0xC0);

				char buffer_segmento_atual[17] = "";
				char pos_no_segmento = 0;
				char max_digitos_segmento = digitos_segmento_alvo[segmento_idx];

				while (1) {
					char tecla = le_tecla();

					if (tecla == '#') {
						if (pos_no_segmento == max_digitos_segmento) {
							strncpy(codigo_boleto_completo + total_pos_boleto, buffer_segmento_atual, pos_no_segmento);
							total_pos_boleto += pos_no_segmento;
							break;
						}
						} else if (tecla == '*') {
						if (pos_no_segmento > 0) {
							pos_no_segmento--;
							buffer_segmento_atual[pos_no_segmento] = '\0';
							lcd_comando(0xC0 + pos_no_segmento);
							lcd_dado(' ');
							lcd_comando(0xC0 + pos_no_segmento);
						}
						} else if (tecla >= '0' && tecla <= '9') {
						if (pos_no_segmento < max_digitos_segmento) {
							if (pos_no_segmento < 16) {
								buffer_segmento_atual[pos_no_segmento] = tecla;
								lcd_dado(tecla);
								pos_no_segmento++;
							}
						}
					}
					if (estado_caixa == 0 || sessao_ativa == 0) return;
				}
			}
			codigo_boleto_completo[total_pos_boleto] = '\0';

			if (total_pos_boleto == 47) {
				char banco_str[4];
				char convenio_str[5];
				char valor_ultimos_10_digitos_str[11];
				char valor_final_para_envio_str[12];

				strncpy(banco_str, codigo_boleto_completo, 3);
				banco_str[3] = '\0';

				strncpy(convenio_str, codigo_boleto_completo + 4, 4);
				convenio_str[4] = '\0';

				strncpy(valor_ultimos_10_digitos_str, codigo_boleto_completo + 37, 10);
				valor_ultimos_10_digitos_str[10] = '\0';

				long valor_long = atol(valor_ultimos_10_digitos_str);
				sprintf(valor_final_para_envio_str, "%ld", valor_long);

				pagamento_aprovado = -1;
				caixa_pagamento(banco_str, convenio_str, valor_final_para_envio_str);

				while (pagamento_aprovado == -1)
				;

				lcd_limpar();
				if (pagamento_aprovado == 1) {
					lcd_string("Pagamento");
					lcd_comando(0xC0);
					lcd_string("efetuado");
					_delay_ms(3000);
					strcpy(ultimo_usuario, nome);
					strcpy(tipo_transacao, "PAGAMENTO");
					long valor_pag_long = atol(valor_final_para_envio_str);
					sprintf(detalhes_transacao, "Banco:%s Convenio:%s Valor:R$%ld,%02ld", banco_str, convenio_str, valor_pag_long / 100, valor_pag_long % 100);
					oferecer_impressao_comprovante();
					} else if (pagamento_aprovado == 0) {
					lcd_string("Saldo");
					lcd_comando(0xC0);
					lcd_string("Insuficiente");
					_delay_ms(3000);
				}
			}
		}
		_delay_ms(10);
	}
}

// Reseta o contador de inatividade (reinicia temporizador)
void reset_inatividade(void) {
	inatividade_segundos = 0;
	piscar_led = 0;
	timer3_stop();
}
