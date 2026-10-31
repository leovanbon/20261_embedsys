#ifndef RC522_H
#define RC522_H
#include <stdbool.h>
#include <stdint.h>
typedef enum { RC522_OK, RC522_NO_CARD, RC522_ERROR, RC522_UNSUPPORTED } RC522_Result;
bool RC522_Init(void);
/* Lab format: four UID bytes + XOR checksum (BCC), not a five-byte UID.
 * Present one short-UID card at a time. Cascaded 7/10-byte UIDs are rejected. */
RC522_Result RC522_ReadCard(uint8_t card[5]);
#endif
