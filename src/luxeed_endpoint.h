#ifndef LUXEED_ENDPOINT_H
#define LUXEED_ENDPOINT_H

#include <stdio.h>
#include <stdlib.h> /* memset(), memcpy() */
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <event.h>

#include "luxeed.h"


typedef struct luxeed_endpoint {
  struct luxeed_options opts;

  int in_fd, out_fd;
  FILE *in, *out;

  int socket_family;
  struct sockaddr *socket_addr;
  socklen_t socket_addr_size;

  struct sockaddr_in inet_addr;
  struct sockaddr_un uds_addr;

  struct event accept_ev;
  struct event read_ev;

  struct event timer_ev;
  struct timeval timer_timeval;
} luxeed_endpoint;

int luxeed_endpoint_init(luxeed_endpoint *ep, int init);
int luxeed_endpoint_open(luxeed_endpoint *ep, int in_fd, int out_fd);
int luxeed_endpoint_bind(luxeed_endpoint *srv);
int luxeed_endpoint_accept(luxeed_endpoint *srv, luxeed_endpoint *cli);
int luxeed_endpoint_close(luxeed_endpoint *ep);


#endif
