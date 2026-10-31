#include "diagnostics.h"
#include "main.h"
#include "ds1307.h"
#include <stdio.h>
#include <string.h>

volatile LabDiagnostics lab_diag;

static void recover_bus(void)
{
    ++lab_diag.recoveries;
    HAL_I2C_DeInit(&hi2c3);
    GPIO_InitTypeDef pin = {0};
    pin.Mode = GPIO_MODE_OUTPUT_OD;
    pin.Pull = GPIO_NOPULL;
    pin.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
    pin.Pin = GPIO_PIN_8;
    HAL_GPIO_Init(GPIOA, &pin);
    pin.Pin = GPIO_PIN_9;
    HAL_GPIO_Init(GPIOC, &pin);
    for (unsigned i = 0; i < 9; ++i) {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_RESET) break;
        if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9) == GPIO_PIN_SET) break;
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_Delay(1);
    }
    /* Generate STOP only if SCL can rise. Outputs remain open-drain. */
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_SET) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
        HAL_Delay(1);
    }
    lab_diag.recovery_released = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8) == GPIO_PIN_SET &&
                                 HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9) == GPIO_PIN_SET;
    HAL_I2C_Init(&hi2c3);
}

void Diagnostics_Scan(void)
{
    if (__HAL_I2C_GET_FLAG(&hi2c3, I2C_FLAG_BUSY) ||
        HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_9) == GPIO_PIN_RESET) recover_bus();
    lab_diag.gpioa = GPIOA->IDR;
    lab_diag.gpioc = GPIOC->IDR;
    lab_diag.gpioe = GPIOE->IDR;
    for (unsigned address = 0; address < 128; ++address) {
        lab_diag.i2c_status[address] = 255;
        if (address >= 8 && address <= 0x77)
            lab_diag.i2c_status[address] = (uint8_t)HAL_I2C_IsDeviceReady(
                &hi2c3, (uint16_t)(address << 1), 1, 5);
    }
    uint8_t raw[8] = {0};
    lab_diag.rtc_status = HAL_I2C_Mem_Read(&hi2c3, 0x68 << 1, 0,
        I2C_MEMADD_SIZE_8BIT, raw, 7, 50);
    lab_diag.i2c_error = HAL_I2C_GetError(&hi2c3);
    /* A factory-stopped clock has no time to preserve. Initialize it once
     * from this firmware's local build time, then it runs on its own. */
    if (lab_diag.rtc_status == HAL_OK && (raw[0] & 0x80)) {
        static const char months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
        char month[4]; unsigned day, year, hour, minute, second;
        if (sscanf(__DATE__, "%3s %u %u", month, &day, &year) == 3 &&
            sscanf(__TIME__, "%u:%u:%u", &hour, &minute, &second) == 3) {
            const char *pos = strstr(months, month);
            if (pos) {
                Time time = {.year=(uint8_t)(year-2000), .month=(uint8_t)((pos-months)/3+1),
                    .day=(uint8_t)day, .hour=(uint8_t)hour, .min=(uint8_t)minute,
                    .sec=(uint8_t)second};
                time.weekday = Time_Weekday(time.year, time.month, time.day);
                SetTime(&time);
            }
        }
    }
    for (unsigned i = 0; i < 8; ++i) lab_diag.rtc_raw[i] = raw[i];
    uint8_t tx[2] = {(0x37 << 1) | 0x80, 0}, rx[2] = {0};
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
    lab_diag.rfid_status = HAL_SPI_TransmitReceive(&hspi4, tx, rx, 2, 10);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
    lab_diag.rfid_version = rx[1];
    lab_diag.spi_error = HAL_SPI_GetError(&hspi4);
    const uint8_t registers[] = {0x01,0x14,0x2A,0x2B,0x2C,0x2D,0x11,0x37};
    for (unsigned i = 0; i < 8; ++i) {
        tx[0] = (uint8_t)((registers[i] << 1) | 0x80);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive(&hspi4, tx, rx, 2, 10);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
        lab_diag.rfid_regs[i] = rx[1];
    }
    ++lab_diag.scans;
}
