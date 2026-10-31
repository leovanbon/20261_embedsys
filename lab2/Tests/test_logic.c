#include "access.h"
#include "console.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

I2C_HandleTypeDef hi2c3;
static uint8_t registers[7];
static HAL_StatusTypeDef bus_status;
static unsigned writes;
static char output[16384];

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *bus, uint16_t address,
    uint16_t reg, uint16_t width, uint8_t *data, uint16_t count, uint32_t timeout)
{
    assert(bus == &hi2c3 && address == 0xD0 && reg == 0 && width == 1 && count == 7 && timeout);
    ++writes;
    if (bus_status == HAL_OK) memcpy(registers, data, 7);
    return bus_status;
}
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *bus, uint16_t address,
    uint16_t reg, uint16_t width, uint8_t *data, uint16_t count, uint32_t timeout)
{
    assert(bus == &hi2c3 && address == 0xD0 && reg == 0 && width == 1 && count == 7 && timeout);
    if (bus_status == HAL_OK) memcpy(data, registers, 7);
    return bus_status;
}

static void capture(const char *text)
{
    assert(strlen(output) + strlen(text) + 2 < sizeof output);
    strcat(output, text);
    strcat(output, "\n");
}
static void command(const char *text)
{
    output[0] = 0;
    for (; *text; ++text) Console_Feed(*text);
    Console_Feed('\r');
    Console_Feed('\n');
}
static const Time valid_time = {.sec=59, .min=58, .hour=23, .weekday=4, .day=29, .month=2, .year=24};
static const uint8_t key[] = {0x11,0x22,0x33,0x44,0x44};
static const uint8_t other[] = {1,2,3,4,4};

static void test_clock(void)
{
    bus_status = HAL_OK;
    assert(SetTime(&valid_time) == HAL_OK);
    const uint8_t expected[] = {0x59,0x58,0x23,4,0x29,0x02,0x24};
    assert(memcmp(registers, expected, 7) == 0);
    Time t;
    assert(GetTime(&t) == HAL_OK && memcmp(&t, &valid_time, sizeof t) == 0);
    assert(Time_Weekday(26,10,31) == 6 && Time_Weekday(0,1,1) == 6);
    t.year = 25;
    unsigned before = writes;
    assert(SetTime(&t) == HAL_ERROR && writes == before); /* Feb 29 in non-leap year */
    t = valid_time; t.weekday = 0;
    assert(SetTime(&t) == HAL_ERROR);
    t = valid_time; t.month = 4; t.day = 31;
    assert(SetTime(&t) == HAL_ERROR);
    t = valid_time; t.hour = 24;
    assert(SetTime(&t) == HAL_ERROR);
    registers[2] = 0x52; /* 12 AM */
    assert(GetTime(&t) == HAL_OK && t.hour == 0);
    registers[2] = 0x72; /* 12 PM */
    assert(GetTime(&t) == HAL_OK && t.hour == 12);
    registers[2] = 0x69; /* 9 PM */
    assert(GetTime(&t) == HAL_OK && t.hour == 21);
    registers[2] = 0x40; /* invalid 0 AM */
    assert(GetTime(&t) == HAL_ERROR);
    memcpy(registers, expected, 7);
    registers[0] |= 0x80; /* CH */
    Time saved = t;
    assert(GetTime(&t) == HAL_ERROR && memcmp(&saved, &t, sizeof t) == 0);
    registers[0] = 0x1A; /* malformed BCD that would otherwise decode as 20 */
    assert(GetTime(&t) == HAL_ERROR);
    memcpy(registers, expected, 7);
    bus_status = HAL_TIMEOUT;
    assert(GetTime(&t) == HAL_TIMEOUT && memcmp(&saved, &t, sizeof t) == 0);
    bus_status = HAL_OK;
    puts("PASS RTC: BCD, 12/24h, calendar, CH, bus errors");
}

static void test_access(void)
{
    AccessControl a;
    Access_Init(&a);
    assert(Access_Scan(&a, key, 0, &valid_time) == ACCESS_REJECTED);
    assert(a.present && !a.granted && a.log_count == 0);
    assert(Access_Add(&a, key) && Access_Add(&a, key) && a.allowed_count == 1);
    assert(Access_Scan(&a, key, 100, &valid_time) == ACCESS_NO_EVENT && !a.granted);
    assert(Access_Scan(&a, NULL, 399, NULL) == ACCESS_NO_EVENT);
    assert(Access_Scan(&a, NULL, 400, NULL) == ACCESS_REMOVED);
    assert(Access_Scan(&a, key, 500, &valid_time) == ACCESS_GRANTED);
    for (unsigned i = 0; i < 50; ++i) assert(Access_Scan(&a, key, 600+i*100, NULL) == ACCESS_NO_EVENT);
    assert(a.log_count == 1 && a.granted);
    assert(Access_LogAt(&a,0)->time_valid);
    assert(Access_Scan(&a, other, 6000, &valid_time) == ACCESS_REJECTED && !a.granted);
    assert(Access_Scan(&a, key, 6100, NULL) == ACCESS_GRANTED);
    assert(!Access_LogAt(&a,1)->time_valid);
    assert(Access_Delete(&a,key) && !a.granted && a.present);
    assert(!Access_Delete(&a,key));
    Access_Init(&a);
    for (unsigned i=0; i<ALLOWED_CAPACITY; ++i) {
        uint8_t id[5] = {(uint8_t)i, 0,0,0,(uint8_t)i};
        assert(Access_Add(&a,id));
    }
    assert(!Access_Add(&a,key));
    Access_ClearCards(&a);
    assert(a.allowed_count == 0);
    assert(Access_Add(&a,key));
    for (unsigned i=0; i<105; ++i) {
        assert(Access_Scan(&a,key,i*1000,&valid_time) == ACCESS_GRANTED);
        assert(Access_Scan(&a,NULL,i*1000+300,NULL) == ACCESS_REMOVED);
    }
    assert(a.log_count == 100 && Access_LogAt(&a,0)->sequence == 6);
    assert(Access_LogAt(&a,99)->sequence == 105 && !Access_LogAt(&a,100));
    Access_ClearLogs(&a);
    assert(a.log_count == 0);
    assert(Access_Scan(&a,key,UINT32_MAX-100,&valid_time) == ACCESS_GRANTED);
    assert(Access_Scan(&a,NULL,100,NULL) == ACCESS_NO_EVENT);
    assert(Access_Scan(&a,NULL,200,NULL) == ACCESS_REMOVED); /* tick rollover */
    puts("PASS access: enroll, reject, hold/remove/change, revoke, 100-log wrap, tick rollover");
}

static void test_console(void)
{
    AccessControl a;
    Access_Init(&a);
    Console_Init(&a,capture);
    command("TIME 2026-10-31 08:30:05");
    assert(strstr(output,"OK TIME"));
    command("TIME");
    assert(strstr(output,"TIME 2026-10-31 08:30:05 DOW=6"));
    unsigned before = writes;
    command("TIME 2025-02-29 00:00:00"); assert(strstr(output,"ERR TIME"));
    command("TIME 2026-10-31 08:30:05garbage"); assert(strstr(output,"ERR TIME"));
    command("TIME 2100-01-01 00:00:00"); assert(writes == before);
    command("ADD 1122334444"); assert(strstr(output,"OK ADD") && a.allowed_count == 1);
    command("ADD 1122334444"); assert(a.allowed_count == 1);
    command("ADD 1122334445"); assert(strstr(output,"ERR CARD"));
    command("ADD 1122334444 00"); assert(strstr(output,"ERR CARD"));
    command("ADD LAST"); assert(strstr(output,"ERR NO_CARD"));
    assert(Access_Scan(&a,key,0,&valid_time) == ACCESS_GRANTED);
    command("LOG"); assert(strstr(output,"BEGIN LOG 1"));
    assert(Access_Scan(&a,other,100,NULL) == ACCESS_REJECTED);
    assert(Access_Scan(&a,key,200,&valid_time) == ACCESS_GRANTED);
    Console_Process(); Console_Process();
    assert(strstr(output,"LOG 1 2024-02-29 23:58:59 1122334444"));
    assert(!strstr(output,"LOG 2 ") && strstr(output,"END LOG")); /* Stable snapshot */
    command("LIST"); assert(strstr(output,"CARD 1122334444") && strstr(output,"END LIST"));
    command("DEL 1122334444"); assert(a.allowed_count == 0 && !a.granted);
    command("ADD LAST"); assert(a.allowed_count == 1);
    command("CLEAR CARDS"); assert(a.allowed_count == 0);
    output[0]=0;
    for (unsigned i=0; i<200; ++i) Console_Feed('X');
    command("ADD 1122334444"); assert(a.allowed_count == 0 && strstr(output,"ERR INPUT"));
    command("ADD 1122334444"); assert(a.allowed_count == 1); /* Recovery */
    Console_Discard();
    command("CLEAR CARDS"); assert(a.allowed_count == 1);
    command("CLEAR LOG"); assert(a.log_count == 0);
    command("LOG"); Console_Process(); assert(strstr(output,"END LOG"));
    command("HELP"); assert(strstr(output,"END HELP"));
    puts("PASS console: strict TIME/UID parsing, commands, snapshot logs, overflow recovery");
}

int main(void)
{
    test_clock(); test_access(); test_console();
    return 0;
}
