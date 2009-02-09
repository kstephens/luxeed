

#include <stdio.h>
#include <stdlib.h> /* memset(), memcpy() */
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h> /* gettimeofday() */
#include "luxeed_device.h"


static int luxeed_dev_id = 0;

static int usb_debug_level = 9;


/* Performance tuning parameters */

/* The time between full frames */
static double min_frame_interval = 0.025;

/* The delay between chunks. */
/* Emperical throttling between chunks */
/* 0.02 secs = 5 I/O frames == 0.1 images/sec */
// double chunk_delay = 0.000750;
double chunk_delay = 0.0;


#if 1
#define RCALL(V,X) do { V = X; if ( V < 0 || dev->opts.debug ) { fprintf(stderr, "  %s => %d\n", #X, (int) V); } } while ( 0 )
#else
#define RCALL(V,X) V = X
#endif


/* Parsed from ep01.txt as initialization, minues the leading 0x02.
   Chunks are sent 65 bytes long.
   Checksum appears at 0x14e.
*/
static unsigned char msg_00[] = {
  /* 0000: */       0x02, 0x01, 0x80, 0x00, 0x01, 0x01, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 
  /* 0010: */ 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0020: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0030: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /*     : */ 0x00,
  /* 0040: */       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /* 0050: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0060: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0070: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /*     : */ 0x00,
  /* 0080: */       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /* 0090: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 00a0: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 00b0: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /*     : */ 0x00,
  /* 00c0: */       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /* 00d0: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 00e0: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 00f0: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /*     : */ 0x00,
  /* 0100: */       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /* 0110: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0120: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0130: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /*     : */ 0x00,
  /* 0140: */       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb0, 
  /* 0150: */ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0160: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  /* 0170: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  /*     : */ 0x00,
};
static int msg_size = sizeof(msg_00);


static unsigned char msg_ff[] = {
    /*             f     0     1     2     3     4     5     6     7     8     9     a     b     c     d     e  */
    /*                                                aa    bb                                                  */
    /* 0000: */       0x02, 0x01, 0x80, 0x00, 0x00, 0x16, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 
    /* 0010: */ 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0020: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0030: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    /*     : */ 0xff,

    /* 0040: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0050: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0060: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0070: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /*     : */ 0xff,

    /* 0080: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0090: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00a0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00b0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /*     : */ 0xff,

    /* 00c0: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00d0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00e0: */ 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    /* 00f0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    /*     : */ 0x00,

    /* 0100: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0110: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    /* 0120: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0130: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /*     : */ 0x00,

    /*                                                                                                      cc,
		  dd											        */
    /* 0140: */       0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe6,
    /* 0150: */ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0160: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0170: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /*     : */ 0x00,
  };


static unsigned char msg_xx[] = {
    /*             f     0     1     2     3     4     5     6     7     8     9     a     b     c     d     e  */
    /*                                                aa    bb                                                  */
    /* 0000: */       0x02, 0x01, 0x80, 0x00, 0x00, 0x16, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 
    /* 0010: */ 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0020: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0030: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    /*     : */ 0xff,

    /* 0040: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0050: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0060: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0070: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /*     : */ 0xff,

    /* 0080: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0090: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00a0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00b0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /*     : */ 0xff,

    /* 00c0: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00d0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 00e0: */ 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
    /* 00f0: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00,
    /*     : */ 0x00,

    /* 0100: */       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    /* 0110: */ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    /* 0120: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0130: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /*     : */ 0x00,

    /*                                                                                                      cc,
		  dd											        */
    /* 0140: */       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe6,
    /* 0150: */ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0160: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* 0170: */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /*     : */ 0x00,
  };


luxeed_device *luxeed_device_create()
{
  luxeed_device *dev = malloc(sizeof(*dev));
  
  memset(dev, 0, sizeof(*dev));
  
  return dev;
}


int luxeed_device_destroy(luxeed_device *dev)
{
  if ( ! dev ) {
    return 0;
  }
  luxeed_device_close(dev);
  free(dev->msg);
  memset(dev, 0, sizeof(*dev));
  free(dev);
  return 0;
}


int luxeed_device_find(luxeed_device *dev, uint16_t vendor, uint16_t product)
{
  struct usb_bus *u_bus;
  struct usb_device *u_dev;
  struct usb_bus *u_busses;

  if ( ! vendor ) {
    vendor  = LUXEED_USB_VENDOR;
  }
  if ( ! product ) {
    product = LUXEED_USB_PRODUCT;
  }

  usb_init();
  usb_find_busses();
  usb_find_devices();
  u_busses = usb_get_busses();

  if ( dev->opts.debug ) {
    usb_set_debug(usb_debug_level);
  }

  for ( u_bus = u_busses; u_bus; u_bus = u_bus->next ) {
    for ( u_dev = u_bus->devices; u_dev; u_dev = u_dev->next ) {
      if ( (u_dev->descriptor.idVendor == vendor) && (u_dev->descriptor.idProduct == product) ) {
	dev->id = luxeed_dev_id ++;

	dev->u_bus = u_bus;
	dev->u_dev = u_dev;

	dev->msg_size = msg_size;
	dev->msg_len = msg_size;
	dev->msg = malloc(sizeof(dev->msg[0]) * dev->msg_size);
	memset(dev->msg, 0, sizeof(dev->msg[0]) * dev->msg_size);

	if ( dev->opts.debug ) {
	  fprintf(stderr, "  bus %s 0x%x\n", (char*) u_bus->dirname, (int) u_bus->location);
	  fprintf(stderr, "  dev %s 0x%x\n", (char*) u_dev->filename, (int) u_dev->devnum);
	}

        return 0;
      }
    }
  }

  return -1;
}


int luxeed_device_open(luxeed_device *dev)
{
  int result = -1;

  do {
    if ( dev->opened ) {
      result = 0;
      break;
    }
    if ( dev->opening ) {
      result = 0;
      break;
    }
    dev->opening = 1;
    dev->opened = 0;

    if ( dev->opts.debug ) {
      fprintf(stderr, "dev opening\n");
    }

    /* Locate the USB device. */
    if ( ! dev->u_dev ) {
      if ( (result = luxeed_device_find(dev, 0, 0)) < 0 ) {
	break;
      }
    }

    /* Open the USB device. */
    dev->u_dh = usb_open(dev->u_dev);
    if ( ! dev->u_dh ) {
      result = -1;
      break;
    }
    
    /* Reset the device */
    RCALL(result, usb_reset(dev->u_dh));
    
    /* Detach any kernel drivers for the endpoint */
    RCALL(result, usb_detach_kernel_driver_np(dev->u_dh, 2));
    
    /* Claim the interface. */
    RCALL(result, usb_claim_interface(dev->u_dh, 2));

    /* Wait for a bit before initializing the device. */
    usleep(100000);

    /* Mark device opened. */
    dev->opening = 0;
    dev->opened = 1;

    if ( dev->opts.debug ) {
      fprintf(stderr, "dev opened\n");
    }

    result = 0;

  } while ( 0 );

  return result;
}


int luxeed_device_close(luxeed_device *dev)
{
  int result = 0;

  do {
    if ( dev->u_dh ) {
      RCALL(result, usb_release_interface(dev->u_dh, 1));
      RCALL(result, usb_release_interface(dev->u_dh, 2));

      RCALL(result, usb_close(dev->u_dh));
      if ( result ) break;

      dev->u_dh = 0;
    }

    /* Force device search on open(). */
    dev->u_dev = 0;

    dev->opened = 0;
    dev->opening = 0;
    dev->inited = 0;
    dev->initing = 0;
  } while ( 0 );

  return result;
}


static int dump_buf(unsigned char *bytes, int size)
{
  int i;

  fprintf(stderr, "\n      ");
  for ( i = 0; i < 0x10; ++ i ) {
    fprintf(stderr, "%2x ", (int) i);
  }
  for ( i = 0; i < size; ++ i ) {
    if ( i % 0x10 == 0 ) {
      fprintf(stderr, "\n%04x: ", (int) i);
    }
    fprintf(stderr, "%02x ", (int) bytes[i]);
  }
  fprintf(stderr, "\n\n");

  return 0;
}


/* 5 blocks * ("0x02" header + 64 data bytes) */
#define LUXEED_BLOCK_SIZE 64

// #define luxeed_send luxeed_send_buffered
#define luxeed_send luxeed_send_chunked


int luxeed_send_chunked (luxeed_device *dev, int ep, unsigned char *bytes, int size)
{
  int result = 0;
  int timeout = 1000;

  usb_dev_handle *dh;

  luxeed_device_msg_checksum(dev, bytes, size);

  if ( dev->opts.debug > 1 ) {
    fprintf(stderr, "send_bytes(%d, %d)...", (int) ep, (int) size);
    dump_buf(bytes, size);
  }


  {
    unsigned char *buf = bytes;
    int left = size;
    int blksize = LUXEED_BLOCK_SIZE;

    while ( left > 0 ) {
      /* Chunk of 0x02 + 64 bytes */
      unsigned char xbuf[1 + blksize];
      int write_result = 0;

      xbuf[0] = 0x02;
      memcpy(xbuf + 1, buf, blksize);

      int wsize = blksize < left ? blksize : left;
      wsize += 1;

      if ( dev->opts.debug > 2 ) {
	dump_buf(xbuf, wsize);
      }

      RCALL(write_result, usb_bulk_write(dev->u_dh, ep, (void*) xbuf, wsize, timeout));
      // RCALL(usb_bulk_write(dh, ep, xbuf, wsize, timeout));

      if ( chunk_delay > 0 ) {
	usleep((int) chunk_delay * 1000000);
      }

      if ( write_result < 0 || write_result != wsize ) {
	result = -1;
	break;
      }
      buf += blksize;
      left -= blksize;
    }
  }
  

  /* Read result */
  if ( 0 && result == 0 ) {
    char buf[1024];
    // int buf_size = sizeof(buf);
    int read_size = 2;
    int read_result = 0;
    RCALL(read_result, usb_bulk_read(dh, ep + 0x80, buf, read_size, timeout));
    if ( read_result != read_size ) {
      fprintf(stderr, "read failed\n");
      // result = -1;
    }
    if ( read_result > 0 ) {
      if ( dev->opts.debug ) {
	fprintf(stderr, "read result:"); dump_buf((unsigned char *) buf, read_result);
      }
      if ( 0 ) {
	assert(buf[0] == 0x01);
	assert(buf[1] == 0x79);
      }
    }
  }

  if ( 0 ) {
    RCALL(result, usb_clear_halt(dh, ep));
  }

  return result;
}



int luxeed_device_msg_checksum(luxeed_device *dev, unsigned char *buf, int size)
{
  int sum = 0;
  int i;
  int sum_save;
  int chksum_i = 0x14e; // 0x154 in total msg output.

  // return 0;

  if ( dev->opts.debug ) {
    if ( buf == msg_ff || buf == msg_00 ) {
      dump_buf(buf, size);
    }
  }
  
  sum_save = buf[chksum_i];
  buf[chksum_i] = 0;

  for ( i = 0; i < size; ++ i ) {
    sum += buf[i];
  }
  sum -= 5;
  sum &= 0xff;

  buf[chksum_i] = sum;

  if ( buf == msg_ff || buf == msg_00 ) {
    if ( sum != sum_save ) {
      dump_buf(buf, size);
      fprintf(stderr, "%p sum %02x, expected %02x\n", (void*) buf, (int) sum, (int) sum_save);
      assert(sum == sum_save);
    }
  } else {
    buf[chksum_i - 1] = 0;
  }

  return sum;
}


/* Initialize all LEDS */
int luxeed_device_init(luxeed_device *dev)
{
  int result = -1;

  do {
    int slp = 100000;
    int i;
    int n = 2;

    if ( dev->initing ) {
      result = 0;
      break;
    }
    if ( dev->inited ) {
      result = 0;
      break;
    }

    if ( dev->opts.debug > 0 ) {
      fprintf(stderr, "dev initing\n");
    }

    dev->initing = 1;
    dev->inited = 0;

    if ( dev->init_count == 0 ) {
      usleep(slp);
      for ( i = 0; i < n; ++ i ) {
	
	RCALL(result, luxeed_send (dev, LUXEED_USB_ENDPOINT_DATA, msg_00, sizeof(msg_00)));
	if ( result ) return result;
	usleep(slp);
	
	RCALL(result, luxeed_send (dev, LUXEED_USB_ENDPOINT_DATA, msg_ff, sizeof(msg_ff)));
	if ( result ) return result;
	usleep(slp);
      }
    }

    RCALL(result, luxeed_send (dev, LUXEED_USB_ENDPOINT_DATA, msg_00, sizeof(msg_00)));
    if ( result ) return result;
    usleep(slp);
      
    dev->initing = 0;
    dev->inited = 1;
    ++ dev->init_count;

    if ( dev->opts.debug > 0 ) {
      fprintf(stderr, "dev: inited\n");
    }

  } while ( 0 );

  return result;
}


const unsigned char *luxeed_device_pixel(luxeed_device *dev, int key)
{
  if ( key < 0 || key >= LUXEED_NUM_OF_KEYS ) {
    return 0;
  }
  return &dev->key_data[key * 3];
}


const unsigned char * luxeed_device_set_key_color(luxeed_device *dev, luxeed_key *key, int r, int g, int b)
{
  unsigned char *p;

  if ( ! key ) return 0;

  p = (unsigned char *) luxeed_device_pixel(dev, key->id);
  if ( ! p ) return 0;
  
  if ( p[0] != r && p[1] != g && p[2] != b ) {
    p[0] = r;
    p[1] = g;
    p[2] = b;
    
    dev->key_data_dirty = 1;  
  }

  return p;
}


int luxeed_device_set_key_color_all(luxeed_device *dev, int r, int g, int b)
{
  int i;

  for ( i = 0; i < LUXEED_NUM_OF_KEYS; ++ i ) {
    luxeed_key *key = luxeed_device_key_by_id(dev, i);
    luxeed_device_set_key_color(dev, key, r, g, b);
  }

  return 0;
}


int luxeed_device_key_color(luxeed_device *dev, luxeed_key *key, int *r, int *g, int *b)
{
 
  const unsigned char *p;

  if ( ! key ) return -1;

  p = (const unsigned char *) luxeed_device_pixel(dev, key->id);
  if ( ! p ) return -1;

  *r = p[0];
  *g = p[1];
  *b = p[2];

  return 0;
}


int luxeed_device_update(luxeed_device *dev, int force)
{
  int result = 0;

  do {
    struct timeval now;

    if ( luxeed_device_open(dev) < 0 ) {
      result = -1;
      break;
    }

    if ( luxeed_device_init(dev) < 0 ) {
      result = -1;
      break;
    }

    /* Only send if key data is dirty. */
    if ( ! (force || dev->key_data_dirty) ) {
      result = 0;
      break;
    }

    /* Send data */
    assert(dev->msg_size == sizeof(msg_xx));

    /* Start with basic msg. */
    memcpy(dev->msg, msg_xx, dev->msg_size);

    /* Copy key pixel data into place. */
    memcpy(dev->msg + 0x37, dev->key_data, sizeof(dev->key_data));

    /* Throttle update frequency. */
    {
       struct timeval then = dev->update_last_send_time;
       gettimeofday(&now, 0);
       if ( then.tv_sec ) {
	double dt = (now.tv_sec - then.tv_sec);
	dt += (now.tv_usec - then.tv_usec) / 1000000.0;
	if ( dev->opts.debug > 0 ) {
	  fprintf(stderr, "  t = %d.%06d  dt = %lg secs\n", (int) now.tv_sec, (int) now.tv_usec, (double) dt);
	}
	if ( min_frame_interval > dt ) {
	  double pause_time = min_frame_interval - dt;
	  if ( dev->opts.debug > 0 ) {
	    fprintf(stderr, "    sleeping for %lg secs\n", (double) pause_time);
	  }
	  usleep(pause_time * 1000000.0);
	}
      }
     }

    /* Compute checksum and send in chunks. */
    result = luxeed_send (dev, LUXEED_USB_ENDPOINT_DATA, dev->msg, dev->msg_size);
    if ( result ) break;

    /* Store the time of completion. */
    gettimeofday(&now, 0);
    dev->update_last_send_time = now;

    /* key_data has been sent. */
    dev->key_data_dirty = 0;

  } while ( 0 );


  /* Try again next time. */
  if ( result < 0 ) {
    luxeed_device_close(dev);
    return 0;
  }

  return result;
}


/*******************************************************************
 * Keys
 */

static struct key_map {
  int offset;
  int row, col;
  const char *map;
  int shift;
} _key_maps[] = {
  { 0 , 0,  0, "`1234567890-=" },
  { 0 , 0,  0, "~!@#$%^&*()_+", 1 }, /* SHIFT */
  { 13, 0, 13, "\01BACKSPACE:\b", 0 },

  { 14, 1,  0, "\01TAB:\t", 0 },
  { 15, 1,  1, "qwertyuiop[]\\", 0 },
  { 15, 1,  1, "QWERTYUIOP{}|", 1 },  /* SHIFT */

  { 28, 2,  0, "\01CAPS ", 0 },
  { 29, 2,  1, "asdfghjkl;'" },
  { 29, 2,  1, "ASDFGHJKL:\"", 1 },  /* SHIFT */
  { 40, 2, 12, "\01ENTER:\n", 1 },

  { 41, 3,  0, "\01LSHIFT ", 0 },
  { 42, 3,  1, "zxcvbnm,./", 0 },
  { 42, 3,  1, "ZXCVBNM<>?", 1 },  /* SHIFT */
  { 52, 3, 11, "\01RSHIFT ", 0 },

  { 53, 4,  0, "\01LCTRL \01LSTART \01LALT ", 0 },
  { 56, 4,  8, "\01RCTRL ", 0 },
  { 58, 4, 11, "\01RALT ", 0 },
  { 60, 4,  9, "\01RSTART \01MENU ", 0 },

  { 67, 0, 15, "\01HOME \01PUP ", 0 },
  { 69, 0, 14, "\01DEL ", 0 },
  { 70, 1, 15, "\01END \01PDOWN ", 0 },
  { 72, 3, 15, "\01UP ", 0 },
  { 73, 4, 13, "\01LEFT \01DOWN \01RIGHT ", 0 },
  { -1, 0 }
};

static luxeed_key _keys[LUXEED_NUM_OF_KEYS + 1];


int luxeed_init_keys()
{
  static int initialized = 0;
  int i;
  struct key_map *map;

  if ( initialized )
    return 0;
  
  initialized ++;

  for ( i = 0; i < LUXEED_NUM_OF_KEYS; ++ i ) {
    luxeed_key *key = &_keys[i];
    key->id = i;
  }
  _keys[LUXEED_NUM_OF_KEYS].id = -1;

  for ( i = 0; (map = &_key_maps[i]) && map->map; ++ i ) {
    const char *s = map->map;
    int k = map->offset;
    int x = map->col;
    int y = map->row;
    // fprintf(stderr, "  key map %d offset: %d\n", i, k);

    while ( *s ) {
      luxeed_key *key;
      int code = 0;
      char name[16];

      memset(name, 0, sizeof(name));

      /* "\01TAB:\t " or "\01CAPS " */
      if ( *s == '\01' ) {
	const char *e = ++ s;
	while ( *e && *e != ' ' && *e != ':' ) {
	  ++ e;
	}
	memcpy(name, s, e - s);

	/* Advance over ':' + char */
	if ( *e == ':' ) {
	  code = e[1];
	  s = (e + 2);
	}
	/* Advance over space. */
	if ( *e == ' ' ) {
	  s = (e + 1);
	}
      }
      else {
	name[0] = *(s ++);
      }

      // fprintf(stderr, "  key: %d %s\n", k, name);

      key = &_keys[k];
      key->id = k;
      key->mapped = 1;
      key->x = x;
      key->y = y;
      {
	int j = 0;
	while ( key->name[j] && strcmp(key->name[j], name) ) {
	  ++ j;
	  assert(j < sizeof(key->name)/sizeof(key->name[0]));
	}
	key->name[j] = strdup(name);
	key->code[j] = code ? code : (name[1] == '\0' ? name[0] : 0);
	key->shift[j] = key->code[j] >= ' ' && map->shift;
      }

      ++ k;
      ++ x;
    }
  }

  return 0;
}


void luxeed_key_map_dump(FILE *out)
{
  int i, j;
  luxeed_key *key;

  luxeed_init_keys();
 
  for ( i = 0; (key = &_keys[i])->id >= 0; ++ i ) {
    if ( ! key->mapped ) continue;
    fprintf(out, "#%02d @%02d,%d", key->id, key->x, key->y);
    for ( j = 0; key->name[j]; ++ j ) {
      if ( key->name[j][0] <= ' ' ) {
	char buf[8];
	snprintf(buf, sizeof(buf), " \\0%o", (int) key->name[j][0]);
	fprintf(out, " %s", buf);
      } else {
	fprintf(out, " %s", key->name[j]);
      }
      fprintf(out, " 0x%02x", key->code[j]);
      fprintf(out, " (%s)", key->shift[j] ? "SHIFT" : "");
    }
    fprintf(out, "\n");
  }
}


luxeed_key *luxeed_device_key_by_id(luxeed_device *dev, int id)
{
  luxeed_key *key = 0;
  int key_i;

  if ( id < 0 || id >= LUXEED_NUM_OF_KEYS )
    return 0;

  luxeed_init_keys();

  for ( key_i = 0; key_i < LUXEED_NUM_OF_KEYS; ++ key_i ) {
    if ( _keys[key_i].id == id ) {
      key = &_keys[key_i];
      break;
    }
  }

  return key;
}


luxeed_key *luxeed_device_key_by_position(luxeed_device *dev, int x, int y)
{
  luxeed_key *key = 0;

  if ( x < 0 || y < 0 ) {
    return 0;
  }

  luxeed_init_keys();

  int key_i;
  for ( key_i = 0; key_i < LUXEED_NUM_OF_KEYS; ++ key_i ) {
    luxeed_key *k = &_keys[key_i];
    if ( k->x == x && k->y == y ) {
      key = k;
      goto done;
    }
  }

 done:
  return key;
}


luxeed_key *luxeed_device_key_by_ascii(luxeed_device *dev, int c)
{
  luxeed_key *key = 0;
  int key_i;
  
  if ( c <= 0 && c > 127 ) {
    return 0;
  }

  luxeed_init_keys();

  for ( key_i = 0; key_i < LUXEED_NUM_OF_KEYS; ++ key_i ) {
    int j;
    luxeed_key *k = &_keys[key_i];
    int code;
    for ( j = 0; (code = key->code[j]); ++ j ) {
      if ( code == c ) { 
	key = k;
	goto done;
      }
    }
  }

 done:
  return key;
}


luxeed_key *luxeed_device_key_by_name(luxeed_device *dev, const char *keyname)
{
  luxeed_key *key = 0;
  int key_i;

  if ( ! keyname || ! *keyname ) 
    return 0;

  luxeed_init_keys();
  
  for ( key_i = 0; key_i < LUXEED_NUM_OF_KEYS; ++ key_i ) {
    int j;
    const char *kn;
    for ( j = 0; (kn = _keys[key_i].name[j]); ++ j ) {
      if ( strcmp(kn, keyname) == 0 ) {
	key = &_keys[key_i];
	goto done;
      }
    }
  }

 done:
  return key;
}



luxeed_key *luxeed_device_key_by_string(luxeed_device *dev, const char *str)
{
  luxeed_key *key = 0;

  if ( ! str || ! *str ) 
    return 0;

  luxeed_init_keys();

  if ( *str == '#' && str[1] ) {
    key = luxeed_device_key_by_id(dev, atoi(str + 1));
  } 
  else if ( *str == '@' && str[1] ) {
    int x = -1;
    int y = -1;
    sscanf(str + 1, "%d,%d", &x, &y);
    key = luxeed_device_key_by_position(dev, x, y);
  } 
  else if ( *str == '0' && str[1] == 'x' && str[2] ) {
    int code = -1;
    sscanf(str + 2, "%x", &code);
    key = luxeed_device_key_by_ascii(dev, code);
  }
  else {
    key = luxeed_device_key_by_name(dev, str);
  }

  return key;
}

/* EOF */
