#ifndef TIMER_H_
#define TIMER_H_

void timer1_ctc_init(void);  // Inicia temporizador de inatividade
void timer3_ctc_init(void);  // Inicia temporizador para piscar LED
void timer3_stop(void);      // Para o timer que pisca LED

#endif /* TIMER_H_ */
