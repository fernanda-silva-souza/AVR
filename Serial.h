#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

// Variável global que indica o estado do caixa
extern volatile char estado_caixa;

// Inicialização da USART
void USART_Init(void);
char USART_Transmit(uint8_t dado);

// Comunicação com o servidor
void caixa_liberado(void);
void caixa_travado(void);
void caixa_data_hora(void);
void caixa_entrada_cliente(char* usuario, char* senha);
void caixa_saldo(void);
void caixa_saque(char* valor_saque);
void caixa_pagamento(char* banco_convenio_valor);
void caixa_boleto_recebido(void);
void imprime_comprovante(void);
void sessao_finalizada(void);
void caixa_operando_normalmente(void);

// OPÇÃO A — Senha silábica
void caixa_troca_senha(char* usuario);

#endif /* SERIAL_H */
