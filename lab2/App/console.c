#include "console.h"
#include <stdio.h>
#include <string.h>

static AccessControl *access;
static ConsoleOutput emit;
static char line[96];
static size_t used;
static bool discard;
static AccessLog snapshot[LOG_CAPACITY];
static size_t dump_count, dump_index;
static bool dumping;

void FormatCard(const uint8_t card[CARD_BYTES], char text[11])
{
    static const char hex[] = "0123456789ABCDEF";
    for (size_t i = 0; i < CARD_BYTES; ++i) {
        text[i*2] = hex[card[i] >> 4];
        text[i*2+1] = hex[card[i] & 15];
    }
    text[10] = '\0';
}

void FormatTime(const Time *t, char text[20])
{
    if (!Time_IsValid(t)) { strcpy(text, "RTC_INVALID"); return; }
    /* Format fixed-width digits to keep output bounded on every C library. */
    const unsigned values[] = {t->year, t->month, t->day, t->hour, t->min, t->sec};
    const unsigned positions[] = {2,5,8,11,14,17};
    memcpy(text, "2000-00-00 00:00:00", 20);
    for (size_t i = 0; i < 6; ++i) {
        text[positions[i]] = (char)('0' + values[i]/10);
        text[positions[i]+1] = (char)('0' + values[i]%10);
    }
}

static int hex_value(char ch)
{
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return -1;
}

static bool parse_card(const char *text, uint8_t card[CARD_BYTES])
{
    if (strlen(text) != 10) return false;
    for (size_t i = 0; i < CARD_BYTES; ++i) {
        int hi = hex_value(text[i*2]), lo = hex_value(text[i*2+1]);
        if (hi < 0 || lo < 0) return false;
        card[i] = (uint8_t)(hi*16 + lo);
    }
    return card[0] != 0x88 && (card[0] ^ card[1] ^ card[2] ^ card[3]) == card[4];
}

static bool parse_time(const char *text, Time *t)
{
    if (strlen(text) != 19 || text[0] != '2' || text[1] != '0' ||
        text[4] != '-' || text[7] != '-' || text[10] != ' ' ||
        text[13] != ':' || text[16] != ':') return false;
    const unsigned positions[] = {2,5,8,11,14,17};
    uint8_t values[6];
    for (size_t i = 0; i < 6; ++i) {
        char a = text[positions[i]], b = text[positions[i]+1];
        if (a < '0' || a > '9' || b < '0' || b > '9') return false;
        values[i] = (uint8_t)((a - '0') * 10 + b - '0');
    }
    *t = (Time){.year=values[0], .month=values[1], .day=values[2],
                .hour=values[3], .min=values[4], .sec=values[5], .weekday=1};
    if (!Time_IsValid(t)) return false;
    t->weekday = Time_Weekday(t->year, t->month, t->day);
    return true;
}

void Console_Init(AccessControl *state, ConsoleOutput output)
{
    access = state;
    emit = output;
    used = 0;
    discard = dumping = false;
    dump_count = dump_index = 0;
}

static void execute(const char *command)
{
    char text[96], card_text[11], time_text[20];
    uint8_t card[CARD_BYTES];
    Time time;
    /* A LOG response has a BEGIN/END pair. Wait for END before another command. */
    if (dumping) { emit("ERR BUSY wait for END LOG"); return; }
    if (strcmp(command, "HELP") == 0) {
        emit("BEGIN HELP");
        emit("TIME | TIME YYYY-MM-DD HH:MM:SS (2000..2099)");
        emit("ADD <10 hex digits> | ADD LAST | DEL <10 hex digits>");
        emit("LIST | CLEAR CARDS | LOG | CLEAR LOG | STATUS | HELP");
        emit("IDs are 4 UID bytes + BCC. Remove/re-present after ADD. RAM resets on boot.");
        emit("END HELP");
    } else if (strcmp(command, "TIME") == 0) {
        if (GetTime(&time) != HAL_OK) { emit("ERR RTC read/clock invalid; set TIME"); return; }
        FormatTime(&time, time_text);
        snprintf(text, sizeof text, "TIME %s DOW=%u", time_text, (unsigned)time.weekday);
        emit(text);
    } else if (strncmp(command, "TIME ", 5) == 0) {
        if (!parse_time(command + 5, &time)) { emit("ERR TIME expected valid YYYY-MM-DD HH:MM:SS"); return; }
        emit(SetTime(&time) == HAL_OK ? "OK TIME" : "ERR RTC write failed");
    } else if (strncmp(command, "ADD ", 4) == 0 || strncmp(command, "DEL ", 4) == 0) {
        bool add = command[0] == 'A';
        if (add && strcmp(command + 4, "LAST") == 0) {
            if (!access->present) { emit("ERR NO_CARD hold a card at reader"); return; }
            memcpy(card, access->current, CARD_BYTES);
        } else if (!parse_card(command + 4, card)) {
            emit("ERR CARD expected 10 hex digits with valid BCC"); return;
        }
        if (add) {
            if (!Access_Add(access, card)) { emit("ERR CARD_LIST_FULL"); return; }
        } else if (!Access_Delete(access, card)) { emit("ERR CARD_NOT_FOUND"); return; }
        FormatCard(card, card_text);
        snprintf(text, sizeof text, "OK %s %s", add ? "ADD" : "DEL", card_text);
        emit(text);
    } else if (strcmp(command, "LIST") == 0) {
        snprintf(text, sizeof text, "BEGIN LIST %u", (unsigned)access->allowed_count);
        emit(text);
        for (size_t i = 0; i < access->allowed_count; ++i) {
            FormatCard(access->allowed[i], card_text);
            snprintf(text, sizeof text, "CARD %s", card_text);
            emit(text);
        }
        emit("END LIST");
    } else if (strcmp(command, "CLEAR CARDS") == 0) {
        Access_ClearCards(access);
        emit("OK CLEAR CARDS");
    } else if (strcmp(command, "CLEAR LOG") == 0) {
        Access_ClearLogs(access);
        emit("OK CLEAR LOG");
    } else if (strcmp(command, "LOG") == 0) {
        dump_count = access->log_count;
        dump_index = 0;
        for (size_t i = 0; i < dump_count; ++i) snapshot[i] = *Access_LogAt(access, i);
        dumping = true;
        snprintf(text, sizeof text, "BEGIN LOG %u", (unsigned)dump_count);
        emit(text);
    } else if (strcmp(command, "STATUS") == 0) {
        snprintf(text, sizeof text, "STATUS cards=%u logs=%u present=%u granted=%u",
                 (unsigned)access->allowed_count, (unsigned)access->log_count,
                 (unsigned)access->present, (unsigned)access->granted);
        emit(text);
    } else emit("ERR COMMAND send HELP");
}

void Console_Discard(void) { used = 0; discard = true; }

void Console_Feed(char ch)
{
    if (ch == '\r' || ch == '\n') {
        if (discard) emit("ERR INPUT discarded; resend complete command");
        else if (used) {
            line[used] = '\0';
            execute(line);
        }
        used = 0;
        discard = false;
        return;
    }
    if (discard) return;
    if (ch == '\b' || ch == 127) { if (used) --used; return; }
    if (ch < 32 || ch > 126 || used == sizeof line - 1) { Console_Discard(); return; }
    line[used++] = ch;
}

void Console_Process(void)
{
    if (!dumping) return;
    if (dump_index == dump_count) {
        emit("END LOG");
        dumping = false;
        return;
    }
    const AccessLog *entry = &snapshot[dump_index++];
    char text[96], card[11], time[20];
    FormatCard(entry->card, card);
    FormatTime(entry->time_valid ? &entry->time : NULL, time);
    snprintf(text, sizeof text, "LOG %lu %s %s", (unsigned long)entry->sequence, time, card);
    emit(text);
}
