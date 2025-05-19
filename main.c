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
	uint8_t pos_usuario = 0, pos_senha = 0;
	char tecla;
	uint8_t tentativas = 0;
	uint8_t usuario_confirmado = 0;

	lcd_limpar();
	lcd_comando(0x80);
	lcd_string("BanrisUFRGS");
	lcd_comando(0xC0);
	lcd_string("Usuario: ");

	while (1) {
		tecla = le_tecla();

		if (tecla == '*') {
			if (!usuario_confirmado) {
				pos_usuario = 0;
				lcd_comando(0xC0);
				lcd_string("Usuario:       ");
				lcd_comando(0xC0 + 9);
				} else {
				pos_senha = 0;
				lcd_comando(0xC0);
				lcd_string("Senha:          ");
				lcd_comando(0xC0 + 7);
			}
		}

		else if (!usuario_confirmado && pos_usuario < 6 && tecla >= '0' && tecla <= '9') {
			usuario[pos_usuario++] = tecla;
			lcd_dado(tecla);
		}

		else if (tecla == '#' && pos_usuario == 6 && !usuario_confirmado) {
			usuario_confirmado = 1;
			pos_senha = 0;
			lcd_comando(0xC0);
			lcd_string("Senha:          ");
			lcd_comando(0xC0 + 7);
		}

		else if (usuario_confirmado && pos_senha < 6 && tecla >= '0' && tecla <= '9') {
			senha[pos_senha++] = tecla;
			lcd_dado('*');
		}

		else if (tecla == '#' && usuario_confirmado && pos_senha == 6) {
			uint8_t i;
			for (i = 0; i < 6; i++) {
				if (usuario[i] != senha[i]) break;
			}

			lcd_limpar();
			lcd_comando(0x80);
			lcd_string("BanrisUFRGS");
			if (i == 6) {
				lcd_comando(0xC0);
				lcd_string("Login completo");
				break;
				} else {
				tentativas++;
				if (tentativas >= 3) {
					lcd_comando(0xC0);
					lcd_string("Acesso negado");
					while (1);
					} else {
					lcd_comando(0xC0);
					lcd_string("Senha invalida");
					_delay_ms(1500);
					pos_senha = 0;
					lcd_comando(0xC0);
					lcd_string("Senha:          ");
					lcd_comando(0xC0 + 7);
				}
			}
		}
	}
}
