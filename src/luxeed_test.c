
#include <stdio.h>
#include <stdlib.h> /* memset(), memcpy() */
#include <time.h> /* time() */
#include <string.h>
#include <assert.h>
#include "luxeed_device.h"


static char *progname;

int main (int argc, char **argv)
{
  int result = 0;
  luxeed_device *dev = 0;

  progname = argv[0];
  
  // dev->debug ++;

  do {
    int i = -1;
    srand(getpid() ^ time(0));
    
    dev = luxeed_device_create();
    
    if ( luxeed_device_find(dev, 0, 0) < 0 ) {
      fprintf(stderr, "%s: luxeed keyboard not found\n", progname);
      break;
    }

    if ( luxeed_device_open(dev) < 0 ) {
      fprintf(stderr, "%s: luxeed keyboard cannot open\n", progname);
      break;
    }


    while ( 1 ) {
      /* int j; */
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
	  unsigned char *pixel = (void *) luxeed_device_pixel(dev, k);
	  pixel[0] = r1;
	  pixel[1] = g1;
	  pixel[2] = b1;
	}

	result = luxeed_device_update(dev, 1); /* force update. */
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
  } while ( 0 );

 done:
  if ( result ) {
    fprintf(stderr, "error: %d\n", result);
  }
  luxeed_device_destroy(dev);
  
  return 0;
}


/* EOF */

