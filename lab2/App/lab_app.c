#include "lab_app.h"
#include "main.h"
#include "access.h"
#include "console.h"
#include "rc522.h"
#include "SH1106.h"
#include "diagnostics.h"
#include <stdio.h>
#include <string.h>

#ifndef LAB_STAGE
#define LAB_STAGE 9
#endif

static AccessControl access;
static Time current_time;
static bool rtc_ok;
static uint32_t clock_tick;
static uint32_t diagnostics_tick;
#if LAB_STAGE >= 7
static bool oled_ok;
static uint32_t display_tick;
#endif
#if LAB_STAGE >= 8
static bool reader_ok;
static uint32_t scan_tick, reader_retry;
static RC522_Result reader_result;
#endif

/* Single producer (USART IRQ), single consumer (main). Never parse or print
 * inside the IRQ: OLED I2C transfers and LOG output can take multiple ms. */
#define RX_CAPACITY 512U
static uint8_t rx_byte;
static volatile uint8_t rx_queue[RX_CAPACITY];
static volatile uint16_t rx_head, rx_tail;
static volatile bool rx_overflow;

static void uart_line(const char *text)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)text, (uint16_t)strlen(text), 100);
    HAL_UART_Transmit(&huart1, (uint8_t *)"\r\n", 2, 10);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart)
{
    if (uart != &huart1) return;
    uint16_t next = (uint16_t)((rx_head + 1) % RX_CAPACITY);
    if (next == rx_tail) rx_overflow = true;
    else { rx_queue[rx_head] = rx_byte; rx_head = next; }
    (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    if (uart != &huart1) return;
    /* HAL ends reception on overrun. Main retries once the handle is ready. */
    rx_overflow = true;
}

static void uart_process(void)
{
    if (rx_overflow) {
        uint32_t saved = __get_PRIMASK();
        __disable_irq();
        rx_tail = rx_head;
        rx_overflow = false;
        __set_PRIMASK(saved);
        Console_Discard();
        uart_line("ERR UART overflow/error; send newline, then resend command");
    }
    if (huart1.RxState == HAL_UART_STATE_READY)
        (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    for (unsigned i = 0; i < 32 && rx_tail != rx_head; ++i) {
        char ch = (char)rx_queue[rx_tail];
        rx_tail = (uint16_t)((rx_tail + 1) % RX_CAPACITY);
        Console_Feed(ch);
    }
    Console_Process();
}

#if LAB_STAGE >= 7
static void oled_line(uint16_t y, const char *text)
{
    SH1106_GotoXY(0, y);
    SH1106_Puts((char *)text, &Font_7x10, SH1106_COLOR_WHITE);
}

static void display(void)
{
    if (!oled_ok) return;
    char text[32], time[20];
    SH1106_Fill(SH1106_COLOR_BLACK);
    if (rtc_ok) {
        FormatTime(&current_time, time);
        oled_line(0, time + 11);
        time[10] = '\0';
        oled_line(12, time);
    } else {
        oled_line(0, "RTC invalid/error");
        oled_line(12, "Set TIME via UART");
    }
#if LAB_STAGE >= 8
    if (!reader_ok) oled_line(26, "RC522 error");
    else if (reader_result == RC522_UNSUPPORTED) oled_line(26, "UID not supported");
    else if (access.present) {
#if LAB_STAGE == 9
        oled_line(26, access.granted ? "Welcome" : "Rejected");
#else
        oled_line(26, "Card found");
#endif
        char card[11];
        FormatCard(access.current, card);
        oled_line(38, card);
    } else oled_line(26, "Scan RFID card");
#endif
    snprintf(text, sizeof text, "Cards:%u Log:%u", (unsigned)access.allowed_count,
             (unsigned)access.log_count);
    oled_line(50, text);
    SH1106_UpdateScreen();
}
#endif

void LabApp_Init(void)
{
    Access_Init(&access); /* Empty allow-list; enroll actual cards over UART. */
    Console_Init(&access, uart_line);
    (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    char text[64];
    snprintf(text, sizeof text, "READY LAB2 stage=%u UART=115200,8N1; send HELP", LAB_STAGE);
    uart_line(text);
    rtc_ok = GetTime(&current_time) == HAL_OK;
    uart_line(rtc_ok ? "RTC OK" : "RTC INVALID/ERROR; set TIME YYYY-MM-DD HH:MM:SS");
#if LAB_STAGE >= 7
    oled_ok = SH1106_Init() != 0;
    uart_line(oled_ok ? "OLED OK" : "OLED ERROR check I2C address/wiring");
#endif
#if LAB_STAGE >= 8
    reader_ok = RC522_Init();
    uart_line(reader_ok ? "RC522 OK" : "RC522 ERROR check SPI/wiring");
#endif
    /* Do not call SetTime at every reset: the battery-backed RTC keeps time. */
    clock_tick = HAL_GetTick() - 500;
}

void LabApp_Process(void)
{
    uart_process();
    uint32_t now = HAL_GetTick();
    if (lab_diag.scans == 0 || (uint32_t)(now - diagnostics_tick) >= 5000) {
        diagnostics_tick = now;
        Diagnostics_Scan();
#if LAB_STAGE >= 7
        if (!oled_ok && lab_diag.i2c_status[0x3C] == HAL_OK)
            oled_ok = SH1106_Init() != 0;
#endif
        now = HAL_GetTick();
    }
    if ((uint32_t)(now - clock_tick) >= 500) {
        clock_tick = now;
        rtc_ok = GetTime(&current_time) == HAL_OK;
#if LAB_STAGE == 6 || LAB_STAGE == 7
        char time[20];
        FormatTime(rtc_ok ? &current_time : NULL, time);
        uart_line(time);
#endif
    }
#if LAB_STAGE >= 8
    if (!reader_ok && (uint32_t)(now - reader_retry) >= 2000) {
        reader_retry = now;
        reader_ok = RC522_Init();
        if (reader_ok) uart_line("RC522 OK recovered");
    }
    if ((uint32_t)(now - scan_tick) >= 100) {
        scan_tick = now;
        uint8_t card[CARD_BYTES];
        reader_result = reader_ok ? RC522_ReadCard(card) : RC522_ERROR;
        if (reader_result == RC522_ERROR && reader_ok) {
            /* Collision/RF noise is also an error; recover reader next interval. */
            reader_ok = false;
            reader_retry = HAL_GetTick();
            uart_line("RC522 ERROR retrying");
        }
        const uint8_t *detected = reader_result == RC522_OK ? card : NULL;
        Time event_time;
        const Time *timestamp = NULL;
        bool new_card = detected && (!access.present || memcmp(access.current, card, CARD_BYTES));
        if (new_card && GetTime(&event_time) == HAL_OK) timestamp = &event_time;
        AccessEvent event = Access_Scan(&access, detected, HAL_GetTick(), timestamp);
        if (event == ACCESS_GRANTED || event == ACCESS_REJECTED) {
            char id[11], text[64];
            FormatCard(card, id);
#if LAB_STAGE == 9
            snprintf(text, sizeof text, "EVENT %s %s%s", access.granted ? "WELCOME" : "REJECTED",
                     id, access.granted && !timestamp ? " RTC_INVALID" : "");
#else
            snprintf(text, sizeof text, "EVENT CARD %s", id);
#endif
            uart_line(text);
        } else if (event == ACCESS_REMOVED) uart_line("EVENT REMOVED");
    }
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, access.present ? GPIO_PIN_SET : GPIO_PIN_RESET);
#if LAB_STAGE == 9
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_14, access.granted ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
#endif
#if LAB_STAGE >= 7
    if ((uint32_t)(now - display_tick) >= 250) {
        display_tick = now;
        display();
    }
#endif
}
