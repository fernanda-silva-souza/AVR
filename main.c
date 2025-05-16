#include <avr/io.h>
#include <util/delay.h>

// === LCD ===

void configura_pinos_lcd() {
	// RS (PH5), E (PH6), D6 (PH3), D7 (PH4)
	DDRH |= (1<<PH3) | (1<<PH4) | (1<<PH5) | (1<<PH6);
	// D4 (PG5)
	DDRG |= (1<<PG5);
	// D5 (PE3)
	DDRE |= (1<<PE3);
}

void envia_para_lcd(char dado) {
	if(dado & (1<<0)) PORTG |= (1<<PG5); else PORTG &= ~(1<<PG5); // D4
	if(dado & (1<<1)) PORTE |= (1<<PE3); else PORTE &= ~(1<<PE3); // D5
	if(dado & (1<<2)) PORTH |= (1<<PH3); else PORTH &= ~(1<<PH3); // D6
	if(dado & (1<<3)) PORTH |= (1<<PH4); else PORTH &= ~(1<<PH4); // D7
}

void pulso_enable() {
	PORTH |= (1<<PH6);
	_delay_us(1);
	PORTH &= ~(1<<PH6);
	_delay_us(1);
}

void lcd_comando(char comando) {
	PORTH &= ~(1<<PH5); // RS = 0
	envia_para_lcd(comando >> 4);
	pulso_enable();
	envia_para_lcd(comando & 0x0F);
	pulso_enable();
	_delay_us(40);
}

void lcd_dado(char dado) {
	PORTH |= (1<<PH5); // RS = 1
	envia_para_lcd(dado >> 4);
	pulso_enable();
	envia_para_lcd(dado & 0x0F);
	pulso_enable();
	_delay_us(40);
}

void inicializa_lcd() {
	configura_pinos_lcd();
	_delay_ms(15);
	lcd_comando(0x28); // 4 bits, 2 linhas
	lcd_comando(0x0C); // display on
	lcd_comando(0x06); // incremento
	lcd_comando(0x01); // limpa
	_delay_ms(2);
}

void lcd_limpar() {
	lcd_comando(0x01);
	_delay_ms(2);
}

void lcd_string(char texto[]) {
	for (char i = 0; i < 16 && texto[i] != 0; i++) {
		lcd_dado(texto[i]);
	}
}

// === TECLADO ===

void atraso_debouncing() {
	_delay_ms(5);
}

char debounce(char coluna) {
	char cont = 0;
	char ultima = 0;
	char atual = 0;

	while (cont < 7) {
		atraso_debouncing();
		atual = (PINK & (1 << coluna));
		if (atual == ultima) {
			cont++;
			} else {
			cont = 0;
			ultima = atual;
		}
	}
	return atual;
}

char le_tecla() {
	char tecla = 0;

	// Linha 1 - PK0
	PORTK &= ~(1 << 0);
	if (debounce(4) == 0) tecla = '1';
	else if (debounce(5) == 0) tecla = '2';
	else if (debounce(6) == 0) tecla = '3';
	PORTK |= (1 << 0);

	// Linha 2 - PK1
	PORTK &= ~(1 << 1);
	if (debounce(4) == 0) tecla = '4';
	else if (debounce(5) == 0) tecla = '5';
	else if (debounce(6) == 0) tecla = '6';
	PORTK |= (1 << 1);

	// Linha 3 - PK2
	PORTK &= ~(1 << 2);
	if (debounce(4) == 0) tecla = '7';
	else if (debounce(5) == 0) tecla = '8';
	else if (debounce(6) == 0) tecla = '9';
	PORTK |= (1 << 2);

	// Linha 4 - PK3
	PORTK &= ~(1 << 3);
	if (debounce(4) == 0) tecla = '*';
	else if (debounce(5) == 0) tecla = '0';
	else if (debounce(6) == 0) tecla = '#';
	PORTK |= (1 << 3);

	return tecla;
}


// === MAIN ===

int main() {
	DDRK = 0x0F; // PK0–PK3 como saída (linhas), PK4–PK6 como entrada (colunas)
	PORTK = 255; // pull-up ativado nas colunas

	inicializa_lcd();

	while (1) {
		char tecla = le_tecla();
		if (tecla != 0) {
			if (tecla == '*') {
				lcd_limpar();
				} else {
				lcd_dado(tecla);
			}
			while (le_tecla() != 0); // espera soltar a tecla
		}
	}
}