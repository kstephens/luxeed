#ifndef LUXEED_H
#define LUXEED_H

#include <stdio.h>

extern const char *luxeed_error_action;

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


#define PDEBUG(x, LEVEL, ARGS ...)					\
  do {									\
    int __level = (LEVEL);						\
    if ( (x)->opts.debug >= __level ) {					\
      fprintf(stderr, "%s: DEBUG %d: %s", (x)->opts.progname, __level, __FUNCTION__); \
      fprintf(stderr, ARGS);						\
      fprintf(stderr, "\n");						\
    }									\
  } while (0)


#endif
