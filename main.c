#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Teclado.h"
#include "LCD.h"
#include <string.h>

int main() {
	DDRK = 0x0F; // PK0–PK3 saída, PK4–PK6 entrada
	PORTK = 0xFF; // pull-up nas colunas

	inicializa_lcd();

	char usuario[7] = "";
	char senha[7] = "";
	uint8_t pos_usuario = 0;
	uint8_t pos_senha = 0;
	char tecla;
	uint8_t modo_senha = 0;  // 0 = digitando usuário, 1 = digitando senha

	lcd_limpar();

	// Linha inicial
	lcd_comando(0x80);
	lcd_string("BanrisUFRGS");

	// Linha "Usuario: "
	lcd_comando(0xC0);
	lcd_string("Usuario: ");

	while (1) {
		tecla = le_tecla();

		// Botão apagar '*'
		if (tecla == '*') {
			if (!modo_senha) {
				pos_usuario = 0;
				memset(usuario, 0, sizeof(usuario));
				lcd_comando(0xC0 + 9);
				for (uint8_t i = 0; i < 6; i++) lcd_dado(' ');
				lcd_comando(0xC0 + 9);
				} else {
				pos_senha = 0;
				memset(senha, 0, sizeof(senha));
				lcd_comando(0xC0);
				lcd_string("Senha:          ");
				lcd_comando(0xC0 + 7);
			}
		}

		// Inserindo usuário
		else if (!modo_senha && pos_usuario < 6 && tecla >= '0' && tecla <= '9') {
			usuario[pos_usuario++] = tecla;
			lcd_dado(tecla);
		}

		// Quando digita '#' após 6 dígitos de usuário
		else if (tecla == '#' && pos_usuario == 6 && !modo_senha) {
			modo_senha = 1;
			lcd_comando(0xC0);
			lcd_string("Senha:          ");
			lcd_comando(0xC0 + 7);
		}

		// Inserindo senha (após habilitar modo_senha)
		else if (modo_senha && pos_senha < 6 && tecla >= '0' && tecla <= '9') {
			senha[pos_senha++] = tecla;
			lcd_dado('*');
		}

		// Finaliza após senha completa e '#' pressionado
		else if (modo_senha && tecla == '#' && pos_senha == 6) {
			lcd_limpar();
			if (strncmp(usuario, senha, 6) == 0) {
				lcd_string("Login completo");
				} else {
				lcd_string("Acesso negado");
			}
			break;
		}

		_delay_ms(300);
	}
}
