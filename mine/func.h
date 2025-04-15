#ifndef __FUNC_H
#define __FUNC_H



#include "stm32g4xx.h"                  // Device header
extern uint8_t Receive;

void LED_scan(void);
void KEY_scan(void);
void USART_scan(void);
void JUDEG(void);
void LCD_scan(void);

#endif
