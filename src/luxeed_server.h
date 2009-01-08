#ifndef LUXEED_SERVER_H
#define LUXEED_SERVER_H

#include "luxeed_device.h"
#include "luxeed_endpoint.h"


typedef struct luxeed_server {
  const char *progname;
  struct luxeed_options opts;

  luxeed_device *dev;

  char server_uri[1024];
  char server_path[1024];

  luxeed_endpoint ep;
} luxeed_server;

int luxeed_server_open(luxeed_server *srv);
int luxeed_server_close(luxeed_server *srv);

void luxeed_server_accept(int fd, short event, void *data);

int luxeed_server_main(int argc, char **argv);


typedef struct luxeed_client {
  struct luxeed_options opts;
  luxeed_server *srv;

  luxeed_endpoint ep;

  int color[3];
} luxeed_client;


int luxeed_client_open(luxeed_client *cli, luxeed_server *srv, int in_fd, int out_fd);
int luxeed_client_close(luxeed_client *cli);

/* Returns -1 on EOF */
int luxeed_read_command(luxeed_client *cli);

void luxeed_client_read(int fd, short event, void *data);

int luxeed_client_main(int argc, char **argv);

#endif
