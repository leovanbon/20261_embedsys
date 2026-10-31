#ifndef ACCESS_H
#define ACCESS_H
#include "ds1307.h"
#include <stddef.h>

#define CARD_BYTES 5U
#define ALLOWED_CAPACITY 16U
#define LOG_CAPACITY 100U
#define CARD_REMOVE_MS 300U

typedef struct {
    uint32_t sequence;
    Time time;
    uint8_t card[CARD_BYTES];
    bool time_valid;
} AccessLog;
typedef enum { ACCESS_NO_EVENT, ACCESS_GRANTED, ACCESS_REJECTED, ACCESS_REMOVED } AccessEvent;
typedef struct {
    uint8_t allowed[ALLOWED_CAPACITY][CARD_BYTES];
    size_t allowed_count;
    AccessLog logs[LOG_CAPACITY];
    size_t log_start, log_count;
    uint32_t next_sequence, last_seen;
    bool present, granted;
    uint8_t current[CARD_BYTES];
} AccessControl;

void Access_Init(AccessControl *a);
bool Access_Add(AccessControl *a, const uint8_t card[CARD_BYTES]);
bool Access_Delete(AccessControl *a, const uint8_t card[CARD_BYTES]);
void Access_ClearCards(AccessControl *a);
void Access_ClearLogs(AccessControl *a);
bool Access_Allowed(const AccessControl *a, const uint8_t card[CARD_BYTES]);
/* card=NULL means a poll missed. time=NULL records RTC unavailable explicitly. */
AccessEvent Access_Scan(AccessControl *a, const uint8_t *card, uint32_t now, const Time *time);
const AccessLog *Access_LogAt(const AccessControl *a, size_t index);
#endif
