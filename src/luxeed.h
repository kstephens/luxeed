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
  int server;
  const char *fifo;
  const char *uds;
  const char *host;
  int port;
  const char *commands[16];
  int commands_n;
} luxeed_options;


#define PDEBUG(x, ARGS ...)					    \
  do {								    \
    if ( (x)->opts.debug ) {						    \
      fprintf(stderr, "%s: DEBUG: %s", (x)->opts.progname, __FUNCTION__);	\
      fprintf(stderr, ARGS);					    \
      fprintf(stderr, "\n");					    \
    }								    \
  } while (0)


#endif
