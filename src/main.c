
/*
sudo apt-get install libusb-dev
*/

#include <stdio.h>
#include <stdlib.h> /* memset(), memcpy() */
#include <string.h>
#include <assert.h>
#include "luxeed.h"


int main (int argc, char **argv)
{
  luxeed_device *dev;

  dev = luxeed_find_device(0, 0);

  //If the keyboard wasn't found the function will have returned NULL
  if ( ! dev ) {
    fprintf(stderr, "luxeed keyboard not found\n");
    return 1;
  }

  {
    int msg_id = -1;
    int result = 0;

    srand(getpid() ^ time(0));
    msg_id ++;

    while ( 1 ) {
      int i = msg_id % LUXEED_NUM_OF_KEYS;
      int j;

      fprintf(stderr, "  i = %d\n", (int) i);

      memset(dev->key_data, 0, sizeof(dev->key_data));

      for ( j = 0; j < 10; ++ j ) {
	int k = (i + j) % LUXEED_NUM_OF_KEYS;
	unsigned char *pixel = dev->key_data + k * 3;

	int r = rand() & 0xff;
	int g = rand() & 0xff;
	int b = rand() & 0xff;

	pixel[0] = r;
	pixel[1] = g;
	pixel[2] = b;
      }

      
#if 0
      for ( j = 0; j < sizeof(dev->key_data); ++ j ) {
	// int c = i % 0xff;
	int c = rand() & 0xff;
	dev->key_data[j] = c;
      }
#endif

      // result = luxeed_ffff(dev);
      result = luxeed_update(dev);
      if ( result ) break;

      // usleep(100000);
      getchar();
      msg_id ++;
    }
  }

  luxeed_destroy(dev);
  
  return 0;
}


/* EOF */

