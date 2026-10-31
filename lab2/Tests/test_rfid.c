#include "rc522.h"
#include "stm32f4xx_hal.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

SPI_HandleTypeDef hspi4;
static uint8_t regs[64], tx_fifo[64], rx_fifo[64];
static unsigned tx_count, rx_count, rx_index, transactions;
static uint32_t tick;
static bool selected, cs_low, spi_fail;
static unsigned fault;
static unsigned irq_reads;
static const uint8_t id[] = {0x11,0x22,0x33,0x44,0x44};
/* Bit-at-a-time CRC reference, independent of production bytewise algorithm. */
static void reference_crc(const uint8_t *bytes, unsigned count, uint8_t crc[2])
{
    uint16_t value=0x6363;
    for (unsigned i=0; i<count; ++i) {
        value ^= bytes[i];
        for (unsigned bit=0; bit<8; ++bit)
            value = (uint16_t)((value >> 1) ^ ((value & 1) ? 0x8408 : 0));
    }
    crc[0]=(uint8_t)value; crc[1]=(uint8_t)(value>>8);
}

static void transact(void)
{
    ++transactions;
    irq_reads=0;
    regs[4]=0x30; regs[6]=0; regs[12]=0;
    rx_index=0; rx_count=0;
    if (fault == 1) { regs[4]=1; return; } /* no card */
    if (fault == 2) { regs[4]=0; return; } /* stuck IRQ, software timeout */
    if (fault == 3) { regs[6]=8; return; } /* collision */
    if (fault == 10) { regs[4]=0x10; return; } /* Idle before timer expiry */
    switch (tx_fifo[0]) {
    case 0x52: /* WUPA must be 7 bits, even on repeated polls. */
        assert(tx_count == 1 && (regs[13] & 7) == 7);
        selected=false;
        rx_fifo[0]=4; rx_fifo[1]=0; rx_count=2;
        break;
    case 0x93:
        assert((regs[13] & 7) == 0);
        if (tx_fifo[1] == 0x20) {
            assert(tx_count == 2);
            memcpy(rx_fifo,id,5); rx_count=5;
            if (fault == 4) rx_count=6; /* oversized response */
            if (fault == 5) rx_count=4; /* short response */
            if (fault == 6) rx_fifo[4]^=1; /* BCC error */
            if (fault == 7) {
                rx_fifo[0]=0x88;
                rx_fifo[4]=rx_fifo[0]^rx_fifo[1]^rx_fifo[2]^rx_fifo[3];
            }
        } else {
            assert(tx_count == 9 && tx_fifo[1] == 0x70);
            assert(memcmp(&tx_fifo[2],id,5) == 0);
            uint8_t crc[2]; reference_crc(tx_fifo,7,crc);
            assert(memcmp(&tx_fifo[7],crc,2) == 0);
            rx_fifo[0] = fault == 8 ? 4 : 8;
            reference_crc(rx_fifo,1,&rx_fifo[1]);
            if (fault == 9) rx_fifo[1]^=1;
            rx_count=3; selected=true;
        }
        break;
    case 0x50: {
        assert(selected && tx_count == 4 && tx_fifo[1] == 0);
        uint8_t crc[2]; reference_crc(tx_fifo,2,crc);
        assert(memcmp(&tx_fifo[2],crc,2) == 0);
        selected=false;
        regs[4]=1; /* HALT succeeds by NOT replying. */
        break;
    }
    default: assert(!"unexpected command");
    }
}

void HAL_GPIO_WritePin(void *port, uint16_t pin, unsigned value)
{
    assert(port == GPIOE && pin == GPIO_PIN_4);
    if (value == GPIO_PIN_RESET) { assert(!cs_low); cs_low=true; }
    else cs_low=false;
}
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *spi, uint8_t *data, uint16_t size, uint32_t timeout)
{
    assert(spi == &hspi4 && cs_low && size == 2 && timeout);
    if (spi_fail) return HAL_TIMEOUT;
    unsigned reg=(data[0]&0x7E)>>1;
    uint8_t value=data[1];
    if (reg == 1 && value == 15) { memset(regs,0,sizeof regs); regs[0x37]=0x92; }
    else if (reg == 4) regs[4] &= (uint8_t)~value;
    else if (reg == 10 && (value & 0x80)) { tx_count=rx_count=rx_index=0; }
    else if (reg == 9) { assert(tx_count < sizeof tx_fifo); tx_fifo[tx_count++]=value; }
    else regs[reg]=value;
    if (reg == 13 && (value & 0x80)) transact();
    return HAL_OK;
}
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *spi, uint8_t *tx, uint8_t *rx,
                                         uint16_t size, uint32_t timeout)
{
    assert(spi == &hspi4 && cs_low && size == 2 && timeout && (tx[0] & 0x80));
    if (spi_fail) return HAL_TIMEOUT;
    unsigned reg=(tx[0]&0x7E)>>1;
    rx[0]=0;
    if (reg == 9) { assert(rx_index < rx_count); rx[1]=rx_fifo[rx_index++]; }
    else if (reg == 10) rx[1]=(uint8_t)rx_count;
    else if (reg == 4 && fault == 10) rx[1]=irq_reads++ == 0 ? 0x10 : 1;
    else rx[1]=regs[reg];
    return HAL_OK;
}
uint32_t HAL_GetTick(void) { return tick++; }
void HAL_Delay(uint32_t delay) { tick+=delay; }

int main(void)
{
    assert(RC522_Init());
    uint8_t card[5];
    for (unsigned i=0; i<3; ++i) {
        assert(RC522_ReadCard(card) == RC522_OK && memcmp(card,id,5) == 0);
        assert(!cs_low && !selected);
    }
    assert(transactions == 12); /* WUPA, ANTICOLL, SELECT, HALT each time */
    for (fault=1; fault<=10; ++fault) {
        memset(card,0xAA,5);
        uint32_t before=tick;
        RC522_Result result=RC522_ReadCard(card);
        if (fault == 1 || fault == 10) assert(result == RC522_NO_CARD);
        else if (fault == 7 || fault == 8) assert(result == RC522_UNSUPPORTED);
        else assert(result == RC522_ERROR);
        assert(tick-before < 100 && !cs_low);
        for (unsigned i=0; i<5; ++i) assert(card[i] == 0xAA);
    }
    spi_fail=true;
    assert(!RC522_Init() && !cs_low);
    assert(RC522_ReadCard(card) == RC522_ERROR && !cs_low);
    puts("PASS RFID: WUPA/select/halt, CRC/BCC, FIFO bounds, unsupported UID, SPI errors/timeouts");
    return 0;
}
