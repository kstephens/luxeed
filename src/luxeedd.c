
#include <stdio.h>
#include <stdlib.h> /* memset(), memcpy() */
#include <string.h>
#include <assert.h>
#include <argp.h>

#include "luxeed_server.h"


const char *argp_program_version =
  "luxeed 1.0";
const char *argp_program_bug_address =
  "<ks.luxeed@kurtstephens.com>";
     
/* Program documentation. */
static char doc[] =
  "luxeed -- Luxeed LED Keyboard Driver";

static struct argp argp = { 0, 0, 0, doc };
     
int main (int argc, char **argv)
{
  argp_parse (&argp, argc, argv, 0, 0, 0);
  luxeed_server_main(argc, argv);
  exit (0);
}

