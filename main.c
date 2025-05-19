#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include "Teclado.h"
#include "LCD.h"

int main() {
	DDRK = 0x0F;
	PORTK = 0xFF;

	inicializa_lcd();

	char usuario[7] = "";
	char senha[7] = "";
	uint8_t pos_usuario = 0;
	uint8_t pos_senha = 0;
	uint8_t modo_senha = 0;
	char tecla;

	lcd_limpar();
	lcd_comando(0x80);
	lcd_string("BanrisUFRGS");

	lcd_comando(0xC0);
	lcd_string("Usuario: ");

	while (1) {
		tecla = le_tecla();

		if (tecla == '*') {
			if (!modo_senha) {
				pos_usuario = 0;
				for (uint8_t i = 0; i < 6; i++) usuario[i] = 0;
				lcd_comando(0xC0 + 9);
				for (uint8_t i = 0; i < 6; i++) lcd_dado(' ');
				lcd_comando(0xC0 + 9);
				} else {
				pos_senha = 0;
				for (uint8_t i = 0; i < 6; i++) senha[i] = 0;
				lcd_comando(0xC0);
				lcd_string("Senha:          ");
				lcd_comando(0xC0 + 7);
			}
		}

		else if (!modo_senha && pos_usuario < 6 && tecla >= '0' && tecla <= '9') {
			usuario[pos_usuario++] = tecla;
			lcd_dado(tecla);
		}

		else if (tecla == '#' && pos_usuario == 6 && !modo_senha) {
			modo_senha = 1;
			lcd_comando(0xC0);
			lcd_string("Senha:          ");
			lcd_comando(0xC0 + 7);
		}

		else if (modo_senha && pos_senha < 6 && tecla >= '0' && tecla <= '9') {
			senha[pos_senha++] = tecla;
			lcd_dado('*');
		}

		else if (modo_senha && tecla == '#' && pos_senha == 6) {
			uint8_t iguais = 1;
			for (uint8_t i = 0; i < 6; i++) {
				if (usuario[i] != senha[i]) {
					iguais = 0;
					break;
				}
			}
			lcd_limpar();
			if (iguais) {
				lcd_string("Login completo");
				} else {
				lcd_string("Acesso negado");
			}
			break;
		}

		_delay_ms(300);
	}
}
