#include "access.h"
#include <string.h>

void Access_Init(AccessControl *a)
{
    memset(a, 0, sizeof *a);
    a->next_sequence = 1;
}

bool Access_Allowed(const AccessControl *a, const uint8_t card[CARD_BYTES])
{
    for (size_t i = 0; i < a->allowed_count; ++i)
        if (memcmp(a->allowed[i], card, CARD_BYTES) == 0) return true;
    return false;
}

bool Access_Add(AccessControl *a, const uint8_t card[CARD_BYTES])
{
    if (Access_Allowed(a, card)) return true;
    if (a->allowed_count == ALLOWED_CAPACITY) return false;
    memcpy(a->allowed[a->allowed_count++], card, CARD_BYTES);
    return true;
}

bool Access_Delete(AccessControl *a, const uint8_t card[CARD_BYTES])
{
    for (size_t i = 0; i < a->allowed_count; ++i) {
        if (memcmp(a->allowed[i], card, CARD_BYTES)) continue;
        --a->allowed_count;
        memcpy(a->allowed[i], a->allowed[a->allowed_count], CARD_BYTES);
        if (a->present && memcmp(a->current, card, CARD_BYTES) == 0) a->granted = false;
        return true;
    }
    return false;
}

void Access_ClearCards(AccessControl *a)
{
    a->allowed_count = 0;
    a->granted = false;
}

void Access_ClearLogs(AccessControl *a)
{
    a->log_start = a->log_count = 0;
    /* Keep sequence monotonic within this boot even after clearing. */
}

static void append_log(AccessControl *a, const uint8_t *card, const Time *time)
{
    size_t index = (a->log_start + a->log_count) % LOG_CAPACITY;
    AccessLog *entry = &a->logs[index];
    *entry = (AccessLog){0};
    entry->sequence = a->next_sequence++;
    memcpy(entry->card, card, CARD_BYTES);
    entry->time_valid = Time_IsValid(time);
    if (entry->time_valid) entry->time = *time;
    if (a->log_count < LOG_CAPACITY) ++a->log_count;
    else a->log_start = (a->log_start + 1) % LOG_CAPACITY;
}

AccessEvent Access_Scan(AccessControl *a, const uint8_t *card, uint32_t now, const Time *time)
{
    if (!card) {
        if (a->present && (uint32_t)(now - a->last_seen) >= CARD_REMOVE_MS) {
            a->present = a->granted = false;
            return ACCESS_REMOVED;
        }
        return ACCESS_NO_EVENT;
    }
    a->last_seen = now;
    /* One event per presentation, including a direct change to another UID. */
    if (a->present && memcmp(a->current, card, CARD_BYTES) == 0) return ACCESS_NO_EVENT;
    memcpy(a->current, card, CARD_BYTES);
    a->present = true;
    a->granted = Access_Allowed(a, card);
    if (!a->granted) return ACCESS_REJECTED;
    append_log(a, card, time);
    return ACCESS_GRANTED;
}

const AccessLog *Access_LogAt(const AccessControl *a, size_t index)
{
    return index < a->log_count ? &a->logs[(a->log_start + index) % LOG_CAPACITY] : NULL;
}
