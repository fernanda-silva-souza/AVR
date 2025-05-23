#ifndef TIMER_H_
#define TIMER_H_

void timer1_ctc_init(void);  // Inicia temporizador de inatividade
void timer3_ctc_init(void);  // Inicia temporizador para piscar LED
void timer3_stop(void);      // Para o timer que pisca LED
void delay_1ms(void);		//Delay 1ms
void delay_2ms(void);		//Delay 2ms
void delay_10ms(void);		//Delay 10ms


#endif /* TIMER_H_ */
