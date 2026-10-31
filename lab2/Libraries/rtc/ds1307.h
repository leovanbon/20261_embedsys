#ifndef DS1307_H
#define DS1307_H
#include <stdbool.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"

/* All fields in this public struct are decimal, NOT BCD. Year = 2000..2099. */
typedef struct Time {
    uint8_t sec, min, hour, weekday, day, month, year;
} Time;

bool Time_IsValid(const Time *time);
/* Monday = 1, ... Sunday = 7. */
uint8_t Time_Weekday(uint8_t year, uint8_t month, uint8_t day);
HAL_StatusTypeDef SetTime(const Time *time);
/* HAL_ERROR for halted clock, malformed BCD or invalid calendar date.
 * On any failure the caller's output remains unchanged. */
HAL_StatusTypeDef GetTime(Time *time);
#endif
