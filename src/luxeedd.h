#ifndef LUXEEDD_H
#define LUXEEDD_H

#include <stdio.h>
#include <stdlib.h> /* memset(), memcpy() */
#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/socket.h>

#include "luxeed.h"
#include "event.h"


typedef struct luxeedd_server {
  const char *progname;

  luxeed_device *dev;

  char server_uri[1024];
  char server_path[1024];

  int server_fd;
  struct sockaddr_un socket_addr;
  socklen_t socket_addr_size;

  int options;
  int debug;

  struct event accept_ev;

} luxeedd_server;

int luxeedd_server_open(luxeedd_server *srv);
int luxeedd_server_close(luxeedd_server *srv);

void luxeedd_server_accept(int fd, short event, void *data);


typedef struct luxeedd_client {
  luxeedd_server *srv;

  int in_fd, out_fd;
  FILE *in, *out;

  int socket_fd;
  struct sockaddr_un socket_addr;
  socklen_t socket_addr_size;

  int options;
  int debug;

  int color[3];

  struct event read_ev;

} luxeedd_client;


int luxeedd_client_open(luxeedd_client *cli, luxeedd_server *srv, int in_fd, int out_fd);
int luxeedd_client_close(luxeedd_client *cli);

/* Returns -1 on EOF */
int luxeed_read_command(luxeedd_client *cli);

void luxeedd_client_read(int fd, short event, void *data);

#endif
