#ifndef LAB2_MAIN_H
#define LAB2_MAIN_H
#include "stm32f4xx_hal.h"
extern I2C_HandleTypeDef hi2c3;
extern SPI_HandleTypeDef hspi4;
extern UART_HandleTypeDef huart1;
void Error_Handler(void);
#endif
