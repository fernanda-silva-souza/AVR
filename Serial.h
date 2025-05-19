#ifndef SERIAL_H
#define SERIAL_H

// Variável global representando o estado do caixa
extern volatile char estado_caixa;

void USART_Init(void);
char USART_Transmit(uint8_t dado);

void caixa_liberado();
void caixa_travado();
void caixa_data_hora();
void caixa_entrada_cliente(char* usuario, char* senha);
void caixa_saldo();
void caixa_saque(char* valor_saque);
void caixa_boleto_recebido();
void caixa_pagamento(char* banco_convenio_valor);
void imprime_comprovante(); // A definir no futuro
void sessao_finalizada();
void caixa_operando_normalmente();

// Implementação opção A
void caixa_troca_senha(char* usuario);

#endif
