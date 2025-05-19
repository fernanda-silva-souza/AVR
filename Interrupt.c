#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "Teclado.h"
#include "LCD.h"
#include "Timers.h"
#include "Serial.h"

// Incluir demais funções .h aqui depois

#define TAM_BUFFER 64  // Tamanho máximo da mensagem a receber
volatile char buffer_rx[TAM_BUFFER];
volatile char idx_rx = 0;
volatile char esperado = 0;  // usado para mensagens com 'n' bytes

#define MAX_CODIGO_BARRAS 60
volatile char codigo_barras[MAX_CODIGO_BARRAS];
volatile char tamanho_codigo = 0;


ISR(USART0_RX_vect) {
  char c = UDR0;

  if (idx_rx < TAM_BUFFER - 1) {
    buffer_rx[idx_rx++] = c;

    // ---------- Mensagens de 2 bytes ----------
    if (idx_rx == 2 && buffer_rx[0] == 'S') {
      if (buffer_rx[1] == 'L') {
        caixa_liberado();  // envia 'C''L'
        idx_rx = 0;
      } else if (buffer_rx[1] == 'T') {
        caixa_travado();  // envia 'C''T'
        idx_rx = 0;
      }
    }

    // ---------- Mensagem de data e hora ----------
    if (idx_rx == 6 && buffer_rx[0] == 'S' && buffer_rx[1] == 'H') {
      char dia = buffer_rx[2];
      char mes = buffer_rx[3];
      char hora = buffer_rx[4];
      char minuto = buffer_rx[5];
      caixa_data_hora();  // envia 'C''H'
      idx_rx = 0;
    }

    // ---------- Mensagens com 'n' dados ----------
    if (idx_rx == 3 && buffer_rx[0] == 'S' && (buffer_rx[1] == 'E' || buffer_rx[1] == 'B')) {
      esperado = buffer_rx[2] + 3;  // cabeçalho + n bytes
    }

    // Resposta à entrada de cliente
    if (buffer_rx[0] == 'S' && buffer_rx[1] == 'E' && esperado > 0 && idx_rx == esperado) {
      if (buffer_rx[2] == 15 && strncmp(&buffer_rx[3], "Nao Autorizado", 15) == 0) {
        lcd_limpar();
        lcd_string("Acesso negado");
      } else {
        lcd_limpar();
        lcd_string("Cliente:");
        lcd_comando(0xC0);
        for (char i = 0; i < buffer_rx[2] && i < 16; i++) {
          lcd_dado(buffer_rx[3 + i]);
        }
      }
      idx_rx = 0;
      esperado = 0;
    }

    // Código de barras (boleto)
    if (buffer_rx[0] == 'S' && buffer_rx[1] == 'B' && esperado > 0 && idx_rx == esperado) {
      tamanho_codigo = buffer_rx[2];

      if (tamanho_codigo <= MAX_CODIGO_BARRAS) {
        for (char i = 0; i < tamanho_codigo; i++) {
          codigo_barras[i] = buffer_rx[3 + i];
        }
      }

      caixa_boleto_recebido();  // envia 'C''B'
      idx_rx = 0;
      esperado = 0;
    }

    // ---------- Mensagens de saque e pagamento ----------
    if (idx_rx == 3 && buffer_rx[0] == 'S' && buffer_rx[1] == 'S') {
      if (buffer_rx[2] == 'O') {
        lcd_limpar();
        lcd_string("Saque realizado");
      } else if (buffer_rx[2] == 'I') {
        lcd_limpar();
        lcd_string("Saldo insuf.");
      }
      idx_rx = 0;
    }

    if (idx_rx == 3 && buffer_rx[0] == 'S' && buffer_rx[1] == 'P') {
      if (buffer_rx[2] == 'O') {
        lcd_limpar();
        lcd_string("Pagamento");
        lcd_comando(0xC0);
        lcd_string("realizado");
      } else if (buffer_rx[2] == 'I') {
        lcd_limpar();
        lcd_string("Saldo insuf.");
      }
      idx_rx = 0;
    }

    // ---------- Mensagem de comprovante ----------
    if (idx_rx == 3 && buffer_rx[0] == 'S' && buffer_rx[1] == 'I') {
      idx_rx = 0;
    }

    // ---------- Finalizar sessão ----------
    if (idx_rx == 2 && buffer_rx[0] == 'S' && buffer_rx[1] == 'F') {
      sessao_finalizada();  // envia 'C''F'
      idx_rx = 0;
    }

    // ---------- Resposta de verificação de saldo ----------
    if (idx_rx == 3 && buffer_rx[0] == 'S' && buffer_rx[1] == 'V') {
      esperado = buffer_rx[2] + 3;  // 3 bytes de cabeçalho + n bytes de saldo
    }

    if (buffer_rx[0] == 'S' && buffer_rx[1] == 'V' && esperado > 0 && idx_rx == esperado) {
      char saldo_str[10];
      char saldo_formatado[16];

      // Copia os dados para saldo_str
      for (i = 0; i < buffer_rx[2]; i++) {
        saldo_str[i] = buffer_rx[3 + i];
      }
      saldo_str[i] = '\0';  // Finaliza a string

      // Converte string de saldo para número inteiro (ex: "27050")
      int valor = 0;
      for (char i = 0; saldo_str[i] != '\0'; i++) {
        valor = valor * 10 + (saldo_str[i] - '0');
      }

      // Monta string no formato R$XXX,XX
      char reais = valor / 100;
      char centavos = valor % 100;
      sprintf(saldo_formatado, "R$%d,%02d", reais, centavos);

      // Mostra no LCD
      lcd_limpar();
      lcd_string("Saldo:");
      lcd_comando(0xC0);
      lcd_string(saldo_formatado);

      // Reseta variáveis de controle
      idx_rx = 0;
      esperado = 0;
    }

    // ---------- Troca de senha ----------
    if (idx_rx == 8 && buffer_rx[0] == 'S' && buffer_rx[1] == 'M') {
      char nova_senha[7];
      for (char i = 0; i < 6; i++) {
        nova_senha[i] = buffer_rx[2 + i];
      }
      nova_senha[6] = '\0';
      lcd_limpar();
      lcd_string("Nova senha:");
      lcd_comando(0xC0);
      lcd_string(nova_senha);
      idx_rx = 0;
    }
  }
}


// FALTOU ADICIONAR O INTERRUPT DO TIMER QUE VERIFICA INATIVIDADE E PISCA LED
