
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

  // dev->debug ++;

  {
    int i = -1;
    srand(getpid() ^ time(0));
    
    while ( 1 ) {
      int j;
      float scale, scale_dir;
      int r, g, b;
      float max;

      do { 
	r = rand() & 0xff;
	g = rand() & 0xff;
	b = rand() & 0xff;
	
	max = r;
	if ( max < g ) max = g;
	if ( max < b ) max = b;

	if ( max != 0 ) {
	  r = LUXEED_COLOR_MAX * (r / max);
	  g = LUXEED_COLOR_MAX * (g / max);
	  b = LUXEED_COLOR_MAX * (b / max);
	}
      } while ( max == 0 );
      // fprintf(stderr, "%02x %02x %02x\n", r, g, b);

      for ( scale = 0, scale_dir = 0.1; ! (scale <= 0 && scale_dir < 0); ) {
	int j;
	int r1, g1, b1;
	r1 = r * scale;
	g1 = g * scale;
        b1 = b * scale;
	// fprintf(stderr, "%02x %02x %02x\n", r1, g1, b1);

	memset(dev->key_data, 0, sizeof(dev->key_data));

	i = (i + 1) % LUXEED_NUM_OF_KEYS;

	for ( j = 0; j < 10; ++ j ) {
	  int k = (i + j) % LUXEED_NUM_OF_KEYS;
	  unsigned char *pixel = luxeed_pixel(dev, k);
	  pixel[0] = r1;
	  pixel[1] = g1;
	  pixel[2] = b1;
	}

	result = luxeed_update(dev);
	if ( result ) goto done;
	usleep(100000);

	scale += scale_dir;
	if ( scale > 1.0 ) {
	  scale_dir = - scale_dir;
	  scale += scale_dir;
	}

	if ( 0 ) {
	  fprintf(stderr, scale_dir < 0 ? "<" : ">");
	  fflush(stderr);
	}
      }
    }
  }

 done:
  if ( result ) {
    fprintf(stderr, "error: %d\n", result);
  }
  luxeed_destroy(dev);
  
  return 0;
}


/* EOF */

