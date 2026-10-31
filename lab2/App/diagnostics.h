#ifndef LAB_DIAGNOSTICS_H
#define LAB_DIAGNOSTICS_H
#include <stdint.h>
/* Read through ST-LINK without needing a USB-UART adapter.
 * i2c_status[address]: 0=ACK, 1=HAL_ERROR, 2=BUSY, 3=TIMEOUT, 255=not scanned. */
typedef struct {
    uint32_t scans, gpioa, gpioc, gpioe, i2c_error, spi_error;
    uint32_t rfid_version, rfid_status, rtc_status;
    uint8_t rtc_raw[8];
    uint8_t i2c_status[128];
    uint32_t recoveries, recovery_released;
    uint8_t rfid_regs[8];
} LabDiagnostics;
extern volatile LabDiagnostics lab_diag;
void Diagnostics_Scan(void);
#endif
