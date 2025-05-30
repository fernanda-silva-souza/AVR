#ifndef LCD_H_
#define LCD_H_

void configura_pinos_lcd(void);
void envia_dados_lcd(char dado);
void pulso_enable(void);
void lcd_comando(char comando);
void lcd_dado(char dado);
void inicializa_lcd(void);
void lcd_limpar(void);
void lcd_string(char texto[]);

#endif /* LCD_H_ */
