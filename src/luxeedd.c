
#include <stdio.h>
#include <stdlib.h> /* memset(), memcpy() */
#include <string.h>
#include <assert.h>
#include "luxeed.h"


static char *progname;

int main (int argc, char **argv)
{
  int result = 0;
  luxeed_device *dev;

  progname = argv[0];
  
  dev = luxeed_find_device(0, 0);

  if ( ! dev ) {
    fprintf(stderr, "%s: luxeed keyboard not found\n", progname);
    return 1;
  }

  do {
    result = luxeed_open(dev);
    if ( result ) break;
		
    fprintf(stdout, "READY\n");

    result = luxeed_read_commands(dev, stdin, stdout);
    if ( result ) break;
  } while ( 0 );

 done:
  if ( result ) {
    fprintf(stderr, "error: %d\n", result);
  }
  luxeed_destroy(dev);
  
  return 0;
}


/* EOF */

