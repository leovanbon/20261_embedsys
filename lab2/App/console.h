#ifndef CONSOLE_H
#define CONSOLE_H
#include "access.h"
typedef void (*ConsoleOutput)(const char *line);
void Console_Init(AccessControl *access, ConsoleOutput output);
void Console_Feed(char ch);
void Console_Discard(void);
/* Emits one snapshot log row per call so the main loop can keep scanning. */
void Console_Process(void);
void FormatCard(const uint8_t card[CARD_BYTES], char text[11]);
void FormatTime(const Time *time, char text[20]);
#endif
