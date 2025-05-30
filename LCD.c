#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Teclado.h"
#include "LCD.h"
#include "Serial.h"
#include "Timer.h"

// Inicializa os pinos do LCD
void configura_pinos_lcd(void) {
	// RS (PH5), E (PH6), D6 (PH3), D7 (PH4)
	DDRH |= (1 << PH3) | (1 << PH4) | (1 << PH5) | (1 << PH6);
	// D4 (PG5)
	DDRG |= (1 << PG5);
	// D5 (PE3)
	DDRE |= (1 << PE3);
}

// Envia os 4 bits para os pinos D4-D7
void envia_dados_lcd(char dado) {
	// D4 - PG5
	if (dado & (1 << 0)) PORTG |= (1 << PG5);
	else PORTG &= ~(1 << PG5);
	// D5 - PE3
	if (dado & (1 << 1)) PORTE |= (1 << PE3);
	else PORTE &= ~(1 << PE3);
	// D6 - PH3
	if (dado & (1 << 2)) PORTH |= (1 << PH3);
	else PORTH &= ~(1 << PH3);
	// D7 - PH4
	if (dado & (1 << 3)) PORTH |= (1 << PH4);
	else PORTH &= ~(1 << PH4);
}

// Pulso no pino E para travar o dado
void pulso_enable(void) {
	PORTH |= (1 << PH6);   // E = 1
	PORTH &= ~(1 << PH6);  // E = 0
}

// Envia comando ao LCD
void lcd_comando(char comando) {
	PORTH &= ~(1 << PH5);           // RS = 0 (comando)
	envia_dados_lcd(comando >> 4);  // parte alta
	pulso_enable();
	envia_dados_lcd(comando & 0x0F);  // parte baixa
	pulso_enable();
	_delay_ms(1);
}

// Envia caractere ao LCD
void lcd_dado(char dado) {
	PORTH |= (1 << PH5);         // RS = 1 (dado)
	envia_dados_lcd(dado >> 4);  // parte alta
	pulso_enable();
	envia_dados_lcd(dado & 0x0F);  // parte baixa
	pulso_enable();
	_delay_ms(1);
}

void inicializa_lcd(void) {
	configura_pinos_lcd();
	_delay_ms(15);
	lcd_comando(0x28);  // 4 bits, 2 linhas, 5x7
	lcd_comando(0x0C);  // display on, cursor off
	lcd_comando(0x06);  // incrementa e não desloca
	_delay_ms(2);
}

void lcd_limpar(void) {
	lcd_comando(0x01);
	_delay_ms(2);
}

void lcd_string(char texto[]) {
	char i = 0;
	for (i = 0; i < 16; i++) {
		lcd_dado(texto[i]);
		if (texto[i + 1] == 0)
		i = 16;
	}
}
