
#include <stdio.h>
#include <stdlib.h> /* memset(), memcpy() */
#include <string.h>
#include <assert.h>
#include <argp.h>

#include "luxeed.h"
#include "luxeed_server.h"

const char *argp_program_version =
  "luxeed 1.0";

const char *argp_program_bug_address =
  "<ks.luxeed@kurtstephens.com>";

static char doc[] =
  "luxeed -- Luxeed LED Keyboard Driver";

static char args_doc[] = "COMMAND ...";

static struct argp_option options[] = {
  { "verbose",  'v', 0,         0,  "Produce verbose output" },
  { "debug",    'd', 0,         0,  "Debug mode" },
  { "debug-syscall", 'D', 0,    0,  "Debug syscalls" },
  { "server",   's', 0,         0,  "Server mode" },
  { "host",     'h', "HOST",    0,  "Hostname or IP address" },
  { "port",     'p', "PORT",    0,  "Port" },
  { "fifo",     'f', "FIFO",    0,  "Named pipe" },
  { "uds",      'u', "UDS",     0,  "UNIX Domain Socket" },
  { "command",  'c', "COMMAND", 0,  "Command" },
  { "keymap",   'k', 0,         0,  "Show keymap" },
  { 0 }
};


static error_t
parse_opt(int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct luxeed_options *opts = state->input;

  switch ( key ) {
  case 'd':
    opts->debug += 1;
    break;

  case 'D':
    opts->debug_syscall += 1;
    break;

  case 'v':
    opts->verbose += 1;
    break;

  case 's':
    opts->server = 1;
    break;

  case 'h':
    opts->host = arg;
    break;

  case 'p':
    opts->port = atoi(arg);
    break;

  case 'k':
    opts->show_key_map = 1;
    break;

  case 'c': case ARGP_KEY_ARG:
    if ( opts->commands_n >= 16 )
      /* Too many opts. */
      argp_usage (state);
    opts->commands[opts->commands_n] = arg;
    break;

#if 0
  case ARGP_KEY_END:
    if (state->arg_num < 2)
      /* Not enough opts. */
      argp_usage (state);
    break;
#endif

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}


static struct argp argp = { options, parse_opt, args_doc, doc };


int main (int argc, char **argv)
{
  int result = 0;

  luxeed_main_argv(argc, argv);

  argp_parse(&argp, argc, argv, 0, 0, &luxeed_options);

  if ( luxeed_options.show_key_map ) {
    luxeed_key_map_dump(stdout);
  } else
  if ( luxeed_options.server ) {
    if ( luxeed_options.debug ) {
      luxeed_log("server mode");
    }
    result = luxeed_server_main(argc, argv);
  } else {
    if ( luxeed_options.debug ) {
      luxeed_log("client mode");
    }
    // result = luxeed_client_main(argc, argv);
  }

  return result ? -1 : 0;
}
