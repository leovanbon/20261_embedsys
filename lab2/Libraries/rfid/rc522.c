/* Bounded, read-only UID driver using the register names supplied with Lab 2.
 * Register behavior: NXP MFRC522 datasheet, revision 3.9.
 * WUPA -> anticollision -> SELECT -> HALT lets a stationary card be read again.
 */
#include "rc522.h"
#include "tm_stm32f4_mfrc522.h"
#include <string.h>

extern SPI_HandleTypeDef hspi4;
static bool io_ok;
volatile uint32_t rc522_init_step, rc522_last_io;
volatile uint32_t rc522_config_check;
volatile uint32_t rc522_last_irq;
volatile uint8_t rc522_exchange_regs[8];

static void write_reg(uint8_t reg, uint8_t value)
{
    if (!io_ok) return;
    uint8_t data[] = {(uint8_t)((reg << 1) & 0x7E), value};
    MFRC522_CS_LOW;
    rc522_last_io = HAL_SPI_Transmit(&hspi4, data, sizeof data, 5);
    io_ok = rc522_last_io == HAL_OK;
    MFRC522_CS_HIGH;
}

static uint8_t read_reg(uint8_t reg)
{
    if (!io_ok) return 0;
    uint8_t tx[] = {(uint8_t)(((reg << 1) & 0x7E) | 0x80), 0};
    uint8_t rx[2] = {0};
    MFRC522_CS_LOW;
    rc522_last_io = HAL_SPI_TransmitReceive(&hspi4, tx, rx, sizeof tx, 5);
    io_ok = rc522_last_io == HAL_OK;
    MFRC522_CS_HIGH;
    return rx[1];
}

static void crc_a(const uint8_t *data, unsigned count, uint8_t out[2])
{
    uint16_t crc = 0x6363;
    for (unsigned i = 0; i < count; ++i) {
        uint8_t b = (uint8_t)(data[i] ^ crc);
        b ^= (uint8_t)(b << 4);
        crc = (uint16_t)((crc >> 8) ^ ((uint16_t)b << 8) ^ ((uint16_t)b << 3) ^ (b >> 4));
    }
    out[0] = (uint8_t)crc;
    out[1] = (uint8_t)(crc >> 8);
}

/* Always check capacity before draining FIFO; malformed RF frames cannot
 * overflow the caller's short ATQA/UID/SAK buffers. */
static RC522_Result exchange(const uint8_t *tx, uint8_t count, uint8_t last_bits,
                            uint8_t *rx, uint8_t capacity, uint16_t *bits)
{
    *bits = 0;
    write_reg(MFRC522_REG_COMMAND, PCD_IDLE);
    write_reg(MFRC522_REG_COMM_IRQ, 0x7F);
    write_reg(MFRC522_REG_FIFO_LEVEL, 0x80);
    write_reg(MFRC522_REG_BIT_FRAMING, last_bits);
    for (uint8_t i = 0; i < count; ++i) write_reg(MFRC522_REG_FIFO_DATA, tx[i]);
    write_reg(MFRC522_REG_COMMAND, PCD_TRANSCEIVE);
    write_reg(MFRC522_REG_BIT_FRAMING, (uint8_t)(last_bits | 0x80));
    uint32_t start = HAL_GetTick();
    RC522_Result result = RC522_ERROR;
    while (io_ok && (uint32_t)(HAL_GetTick() - start) < 40) {
        uint8_t irq = read_reg(MFRC522_REG_COMM_IRQ);
        rc522_last_irq = irq;
        /* IdleIRQ alone does not prove that a frame was received. */
        if (irq & 0x20) {
            /* Buffer overflow, collision, parity and protocol errors. */
            if (read_reg(MFRC522_REG_ERROR) & 0x1B) break;
            uint8_t n = read_reg(MFRC522_REG_FIFO_LEVEL);
            uint8_t tail = read_reg(MFRC522_REG_CONTROL) & 7;
            if (n == 0 || n > capacity) break;
            *bits = (uint16_t)(tail ? (n - 1) * 8 + tail : n * 8);
            for (uint8_t i = 0; i < n; ++i) rx[i] = read_reg(MFRC522_REG_FIFO_DATA);
            result = RC522_OK;
            break;
        }
        if (irq & 0x01) { result = RC522_NO_CARD; break; }
    }
    const uint8_t debug_regs[] = {0x06,0x01,0x14,0x2A,0x2B,0x2C,0x2D,0x0D};
    for (unsigned j = 0; j < 8; ++j) rc522_exchange_regs[j] = read_reg(debug_regs[j]);
    write_reg(MFRC522_REG_BIT_FRAMING, last_bits);
    write_reg(MFRC522_REG_COMMAND, PCD_IDLE);
    return io_ok ? result : RC522_ERROR;
}

bool RC522_Init(void)
{
    rc522_init_step = 1;
    io_ok = true;
    MFRC522_CS_HIGH;
    write_reg(MFRC522_REG_COMMAND, PCD_RESETPHASE);
    if (!io_ok) return false;
    rc522_init_step = 2;
    HAL_Delay(50);
    uint32_t start = HAL_GetTick();
    while (read_reg(MFRC522_REG_COMMAND) & 0x10) {
        if (!io_ok || (uint32_t)(HAL_GetTick() - start) >= 100) return false;
    }
    uint8_t version = read_reg(MFRC522_REG_VERSION);
    rc522_init_step = 3;
    if (!io_ok || version == 0 || version == 0xFF) return false;
    write_reg(MFRC522_REG_T_MODE, 0x80);
    write_reg(MFRC522_REG_T_PRESCALER, 0xA9); /* 13.56 MHz / 339 = 40 kHz */
    write_reg(MFRC522_REG_T_RELOAD_H, 0x03);
    write_reg(MFRC522_REG_T_RELOAD_L, 0xE8); /* 1000 counts = 25 ms */
    write_reg(MFRC522_REG_TX_MODE, 0);
    write_reg(MFRC522_REG_RX_MODE, 0);
    write_reg(MFRC522_REG_TX_AUTO, 0x40);
    write_reg(MFRC522_REG_MODE, 0x3D);
    write_reg(MFRC522_REG_RF_CFG, 0x70);
    uint8_t antenna = read_reg(MFRC522_REG_TX_CONTROL);
    write_reg(MFRC522_REG_TX_CONTROL, (uint8_t)(antenna | 3));
    HAL_Delay(5);
    rc522_config_check = 0;
    if (read_reg(MFRC522_REG_T_PRESCALER) != 0xA9) rc522_config_check |= 1;
    if (read_reg(MFRC522_REG_T_RELOAD_H) != 0x03) rc522_config_check |= 2;
    if (read_reg(MFRC522_REG_T_RELOAD_L) != 0xE8) rc522_config_check |= 4;
    if (read_reg(MFRC522_REG_MODE) != 0x3D) rc522_config_check |= 8;
    rc522_init_step = 4;
    return io_ok && rc522_config_check == 0;
}

RC522_Result RC522_ReadCard(uint8_t card[5])
{
    io_ok = true;
    uint8_t request = PICC_REQALL; /* Wake HALTed cards as well as idle cards. */
    uint8_t response[5];
    uint16_t bits;
    RC522_Result result = exchange(&request, 1, 7, response, sizeof response, &bits);
    if (result != RC522_OK) return result;
    if (bits != 16) return RC522_ERROR;
    const uint8_t anti[] = {PICC_ANTICOLL, 0x20};
    result = exchange(anti, sizeof anti, 0, response, sizeof response, &bits);
    if (result != RC522_OK) return result;
    if (bits != 40 || (response[0] ^ response[1] ^ response[2] ^ response[3]) != response[4])
        return RC522_ERROR;
    if (response[0] == 0x88) return RC522_UNSUPPORTED; /* Cascade tag */
    uint8_t select[9] = {PICC_SElECTTAG, 0x70};
    memcpy(&select[2], response, 5);
    crc_a(select, 7, &select[7]);
    result = exchange(select, sizeof select, 0, response, sizeof response, &bits);
    if (result != RC522_OK) return result;
    if (bits != 24) return RC522_ERROR;
    uint8_t crc[2];
    crc_a(response, 1, crc);
    if (response[1] != crc[0] || response[2] != crc[1]) return RC522_ERROR;
    if (response[0] & 4) return RC522_UNSUPPORTED;
    uint8_t halt[4] = {PICC_HALT, 0};
    crc_a(halt, 2, &halt[2]);
    /* A correctly halted card is silent, hence timer expiry is success here. */
    result = exchange(halt, sizeof halt, 0, response, sizeof response, &bits);
    if (result != RC522_NO_CARD) return RC522_ERROR;
    memcpy(card, &select[2], 5);
    return RC522_OK;
}
