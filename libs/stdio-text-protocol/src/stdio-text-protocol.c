#include "stdio-text-protocol/stdio-text-protocol.h"
#include "pico/stdlib.h"
#include <stddef.h>
#include <stdio.h>
static char line[STDIO_TEXT_PROTOCOL_LINE_SIZE];
static unsigned used;
static bool overflow, completed;
static command_t command;
void stdio_text_protocol_init(void) { used=0; overflow=false; completed=false; }
const command_t *stdio_text_protocol_handle(void)
{
    if (completed) { used=0; overflow=false; completed=false; }
    int c=getchar_timeout_us(0);
    if (c<0) return NULL;
    if (c=='\r' || c=='\n') {
        if (!used && !overflow) return NULL;
        puts("");
        line[used]='\0'; completed=true;
        command=(command_t){.truncated=overflow};
        char *p=line;
        while (*p) {
            while (*p==' ') ++p;
            if (!*p) break;
            char *word=p;
            while (*p && *p!=' ') ++p;
            if (*p) *p++='\0';
            if (!command.name) command.name=word;
            else if (command.argc<STDIO_TEXT_PROTOCOL_MAX_ARGS) command.argv[command.argc++]=word;
            else command.truncated=true;
        }
        return command.name || command.truncated ? &command : NULL;
    }
    if (c==0x08 || c==0x7f) {
        if (used) { --used; overflow=false; fputs("\b \b",stdout); }
    } else if (c>=0x20 && c<=0x7e) {
        if (used<sizeof(line)-1) { line[used++]=(char)c; putchar(c); }
        else overflow=true;
    }
    return NULL;
}
