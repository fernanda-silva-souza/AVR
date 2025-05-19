#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Teclado.h"
#include "LCD.h"

int main(){
	DDRK = 0x0F; // PK0–PK3 como saída (linhas), PK4–PK6 como entrada (colunas)
	PORTK = 255; // pull-up ativado nas colunas

	inicializa_lcd();									// 	Inicializa o LCD
	
	char usuario[7] = "";    // Até 6 dígitos + '\0'
	uint8_t pos = 0;
	char tecla;

	lcd_limpar();

	// Primeira linha: fixo
	lcd_comando(0x80);
	lcd_string("BanrisUFRGS");

	// Segunda linha: "Usuario: "
	lcd_comando(0xC0);
	lcd_string("Usuario: ");

	while (1) {
		tecla = le_tecla();
		if (tecla == '*') {
			// Apaga toda a senha digitada
			pos = 0;
			//usuario[0] = '\0';
			// Limpa os asteriscos após "Usuario: "
			lcd_comando(0xC0 + 9);
			for (char i = 0; i < 6; i++) {
				lcd_dado(' ');
			}
			lcd_comando(0xC0 + 9); // Reposiciona o cursor após "Usuario: "
		}
		if (tecla >= '0' && tecla <= '9') {
			if (pos < 6) {
				usuario[pos++] = tecla;
				//usuario[pos] = '\0';
				// Escreve todos os asteriscos novamente (limpa erros e sobreposições)
				lcd_dado(tecla);
			}
		}
	}
}
