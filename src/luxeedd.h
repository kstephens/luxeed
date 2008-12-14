#ifndef LUXEEDD_H
#define LUXEEDD_H

#include <stdio.h>
#include <stdlib.h> /* memset(), memcpy() */
#include <string.h>
#include <assert.h>

#include "luxeed.h"
#include "event.h"


typedef struct luxeedd_client {
  luxeed_device *dev;

  int in_fd, out_fd;
  FILE *in, *out;

  int options;

  int color[3];

  struct event read_ev;

} luxeedd_client;

/* Returns -1 on EOF */
int luxeed_read_command(luxeed_client *cli);

#endif
