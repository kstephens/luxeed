#ifndef LUXEED_H
#define LUXEED_H

#include <stdio.h>

extern
struct luxeed_options
{
  const char *progname;
  int verbose;
  int debug;
  int debug_syscall;
  int server;
  const char *fifo;
  const char *uds;
  const char *host;
  int port;
  const char *commands[16];
  int commands_n;
  int show_key_map;
} luxeed_options;

extern const char *luxeed_progname;
extern int luxeed_log_level;
extern const char *luxeed_error_action;

void luxeed_main_argv(int argc, char** argv);
void luxeed_logv_(const char* file, int line, const char* function, const char* fmt, va_list* vap);
void luxeed_log_(const char* file, int line, const char* function, const char* fmt, ...);
#define luxeed_log(FMT, ...) luxeed_log_(__FILE__, __LINE__, __FUNCTION__, FMT, ##__VA_ARGS__)
#define luxeed_error(FMT, ...) luxeed_log("ERROR: " FMT, ##__VA_ARGS__)

#define PDEBUG(x, LEVEL, FMT, ...)                                \
  do {                                                            \
    int __luxeed_level = (LEVEL);                                 \
    if ( (x)->opts.debug >= __luxeed_level ) {                    \
      luxeed_progname = (x)->opts.progname ?: luxeed_progname;    \
      luxeed_log("DEBUG %d : " FMT, __luxeed_level, ##__VA_ARGS__); \
    }                                                             \
  } while (0)

#endif
