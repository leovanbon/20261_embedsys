#ifndef FAKE_HAL_H
#define FAKE_HAL_H
#include <stdint.h>
typedef enum {HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT} HAL_StatusTypeDef;
typedef struct {unsigned unused;} I2C_HandleTypeDef;
typedef struct {unsigned unused;} SPI_HandleTypeDef;
#define I2C_MEMADD_SIZE_8BIT 1
#define GPIOE ((void *)1)
#define GPIO_PIN_4 16
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *, uint16_t, uint16_t, uint16_t, uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *, uint16_t, uint16_t, uint16_t, uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *, uint8_t *, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *, uint8_t *, uint8_t *, uint16_t, uint32_t);
void HAL_GPIO_WritePin(void *, uint16_t, unsigned);
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t);
#endif
