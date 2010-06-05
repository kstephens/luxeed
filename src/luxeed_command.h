#ifndef LUXEED_COMMAND_H
#define LUXEED_COMMAND_H

#include "luxeed_server.h"

int luxeed_client_run_command(luxeed_client *cli, char *buf, char *out_buf, size_t out_buf_size, int *force_outputp, char **cmdp, char **errorp, char **error2p);

#endif

/* EOF */
