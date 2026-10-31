#include "ds1307.h"
#include <stddef.h>

extern I2C_HandleTypeDef hi2c3;
#define DS1307_ADDRESS (0x68U << 1) /* HAL supplies the R/W bit itself. */

bool Time_IsValid(const Time *t)
{
    static const uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (!t || t->year > 99 || t->month < 1 || t->month > 12 ||
        t->weekday < 1 || t->weekday > 7 || t->hour > 23 ||
        t->min > 59 || t->sec > 59) return false;
    unsigned limit = days[t->month - 1];
    if (t->month == 2 && t->year % 4 == 0) ++limit;
    return t->day >= 1 && t->day <= limit;
}

uint8_t Time_Weekday(uint8_t year, uint8_t month, uint8_t day)
{
    static const unsigned offsets[] = {0,3,2,5,0,3,5,1,4,6,2,4};
    if (month < 1 || month > 12) return 0;
    unsigned y = 2000U + year;
    if (month < 3) --y;
    unsigned d = (y + y/4 - y/100 + y/400 + offsets[month-1] + day) % 7;
    return (uint8_t)(d == 0 ? 7 : d);
}

static uint8_t to_bcd(uint8_t n) { return (uint8_t)((n / 10) * 16 + n % 10); }
static uint8_t from_bcd(uint8_t n) { return (uint8_t)((n >> 4) * 10 + (n & 15)); }
static bool valid_bcd(uint8_t n) { return (n & 15) <= 9 && (n >> 4) <= 9; }

HAL_StatusTypeDef SetTime(const Time *t)
{
    if (!Time_IsValid(t)) return HAL_ERROR;
    uint8_t raw[7] = {to_bcd(t->sec), to_bcd(t->min), to_bcd(t->hour),
                     t->weekday, to_bcd(t->day), to_bcd(t->month), to_bcd(t->year)};
    /* CH=0 starts oscillator; hour bit 6=0 selects 24-hour mode. */
    return HAL_I2C_Mem_Write(&hi2c3, DS1307_ADDRESS, 0, I2C_MEMADD_SIZE_8BIT,
                             raw, sizeof raw, 100);
}

HAL_StatusTypeDef GetTime(Time *t)
{
    if (!t) return HAL_ERROR;
    uint8_t raw[7];
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c3, DS1307_ADDRESS, 0,
                                  I2C_MEMADD_SIZE_8BIT, raw, sizeof raw, 100);
    if (status != HAL_OK) return status;
    /* Check CH and reserved bits before masking mode bits. */
    if ((raw[0] & 0x80) || (raw[1] & 0x80) || (raw[2] & 0x80) ||
        (raw[3] & 0xF8) || (raw[4] & 0xC0) || (raw[5] & 0xE0)) return HAL_ERROR;
    uint8_t hour = raw[2] & ((raw[2] & 0x40) ? 0x1F : 0x3F);
    if (!valid_bcd(raw[0]) || !valid_bcd(raw[1]) || !valid_bcd(hour) ||
        !valid_bcd(raw[4]) || !valid_bcd(raw[5]) || !valid_bcd(raw[6])) return HAL_ERROR;
    hour = from_bcd(hour);
    if (raw[2] & 0x40) {
        if (hour < 1 || hour > 12) return HAL_ERROR;
        hour = (uint8_t)(hour % 12 + ((raw[2] & 0x20) ? 12 : 0));
    }
    Time result = {from_bcd(raw[0]), from_bcd(raw[1]), hour, raw[3],
                   from_bcd(raw[4]), from_bcd(raw[5]), from_bcd(raw[6])};
    if (!Time_IsValid(&result)) return HAL_ERROR;
    *t = result;
    return HAL_OK;
}
