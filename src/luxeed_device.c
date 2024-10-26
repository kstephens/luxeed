

#include <stdio.h>
#include <stdlib.h> /* memset(), memcpy() */
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h> /* gettimeofday() */
#include <unistd.h> // usleep()
#include "luxeed_device.h"
#include "luxeed_key.h"
#include "luxeed_usb.c"


static int luxeed_dev_id = 0;
static int usb_debug_level = 9;


/* Performance tuning parameters */

/* The time between full frames */
static double min_frame_interval = 0.025;

/* The delay between chunks. */
/* Emperical throttling between chunks */
/* 0.02 secs = 5 I/O frames == 0.1 images/sec */
double chunk_delay = 0.000750;
// double chunk_delay = 0.0;


#if 1
#define RCALL(V,X) ({ V = X; if ( dev->opts.debug_syscall >= 1 ) { luxeed_log("  %s => %ld", #X, (long) V); }; V; })
#else
#define RCALL(V,X) ({ V = X })
#endif

/* Parsed from ep01.txt as initialization, minus the leading 0x02.
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
  dev->id = luxeed_dev_id ++;

  return dev;
}


int luxeed_device_destroy(luxeed_device *dev)
{
  if ( ! dev ) {
    return 0;
  }
  PDEBUG(dev, 1, "(%p)", dev);

  luxeed_device_close(dev);
  free(dev->msg);
  memset(dev, 0, sizeof(*dev));
  free(dev);

  return 0;
}


int luxeed_device_find(luxeed_device *dev, int vendor, int product) {
  int result = 0;
  int usb_result = 0;
  libusb_device **devs = 0;

  dev->u_dev = 0;
  memset(&dev->u_dev_desc, 0, sizeof(dev->u_dev_desc));

  PDEBUG(dev, 1, "(%p, %d, %d)", dev, vendor, product);

  if ( dev->opts.debug_syscall >= 1 ) {
    libusb_set_debug(dev->u_ctx, usb_debug_level);
  }

  ssize_t n_devices = 0;
  RCALL(n_devices, libusb_get_device_list(dev->u_ctx, &devs));

  libusb_device* u_dev = 0;
  for ( int dev_i = 0; (u_dev = devs[dev_i]); dev_i ++) {
		struct libusb_device_descriptor desc;
		RCALL(usb_result, libusb_get_device_descriptor(u_dev, &desc));
    if ( usb_result ) {
      luxeed_error("libusb_get_device_descriptor() failed : %d", usb_result);
      result = -1;
    }

    int found = desc.idVendor == vendor && desc.idProduct == product;
    if ( found ) {
      dev->u_dev = u_dev;
      dev->u_dev_desc = desc;
    }

		luxeed_log("device %d : %04x:%04x (bus %d, device %d) : %p%s\n",
      dev_i,
			desc.idVendor, desc.idProduct,
			libusb_get_bus_number(u_dev), libusb_get_device_address(u_dev),
      u_dev,
      found ? " <<<<" : ""
    );
  }

  if ( ! dev->u_dev ) {
    luxeed_error("device not found");
    result = -1;
  }

  if ( devs )
    libusb_free_device_list(devs, 1);

  PDEBUG(dev, 1, "(%p, %d, %d) => %d", dev, vendor, product, result);

  return result;
}

int luxeed_device_open(luxeed_device *dev)
{
  int result = 0;
  int usb_result = 0;
  int vendor = LUXEED_USB_VENDOR;
  int product = LUXEED_USB_PRODUCT;

  if ( dev->opened ) {
    PDEBUG(dev, 2, "(%p) : already open", dev);
    return 0;
  }
  if ( dev->opening ) {
    PDEBUG(dev, 2, "(%p) : already opening", dev);
    return 0;
  }

  PDEBUG(dev, 1, "(%p)", dev);
  do {
    assert(dev->u_ctx);
    dev->u_dev_hd = 0;
    dev->opening = 1;
    dev->opened = 0;
    PDEBUG(dev, 1, "opening");

    result = luxeed_device_find(dev, vendor, product);
    if ( result ) {
      result = -1;
      break;
    }

    RCALL(usb_result, libusb_open(dev->u_dev, &dev->u_dev_hd));
    if ( usb_result ) {
      luxeed_error("libusb_open(%p, %p): failed : %d", dev->u_ctx, &dev->u_dev_hd, usb_result);
      result = -1;
      break;
    }

    dev->u_dev = libusb_get_device(dev->u_dev_hd);
    dev->msg_size = msg_size;
    dev->msg_len = msg_size;
    dev->msg = malloc(sizeof(dev->msg[0]) * dev->msg_size);
    memset(dev->msg, 0, sizeof(dev->msg[0]) * dev->msg_size);

    /* Reset the device */
    RCALL(result, libusb_reset_device(dev->u_dev_hd));

    /* Claim the interface. */
    RCALL(usb_result, libusb_claim_interface(dev->u_dev_hd, LUXEED_USB_INTERFACE));

    /* Wait for a bit before initializing the device. */
    // usleep(100000);

    /* Mark device opened. */
    dev->opening = 0;
    dev->opened = 1;
    luxeed_device_show(dev);
  } while(0);

  PDEBUG(dev, 1, "(%p, %d, %d) => %d", dev, vendor, product, result);

  return result;
}


int luxeed_device_opened(luxeed_device *dev)
{
  return dev ? dev->opened : 0;
}


int luxeed_device_close(luxeed_device *dev)
{
  int result = 0;

  PDEBUG(dev, 1, "(%p)", dev);

  do {
    if ( dev->u_dev_hd ) {
      RCALL(result, libusb_release_interface(dev->u_dev_hd, LUXEED_USB_INTERFACE));
      libusb_close(dev->u_dev_hd);
      dev->u_dev_hd = 0;
    }

    dev->u_dev = 0;
    memset(&dev->u_dev_desc, 0, sizeof(dev->u_dev_desc));

    dev->opened = 0;
    dev->opening = 0;
    dev->inited = 0;
    dev->initing = 0;
  } while ( 0 );

  PDEBUG(dev, 1, "(%p) => %d", dev, result);

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


int luxeed_send_chunked (luxeed_device *dev, int ep_out, unsigned char *bytes, int size)
{
  int result = 0;
  int timeout = 1000;
  // timeout = 0;

  PDEBUG(dev, 2, "(%p, %d, %p, %d)", dev, ep_out, bytes, size);

  do {
  luxeed_device_msg_checksum(dev, bytes, size);

  if ( dev->opts.debug_syscall >= 2 ) {
    fprintf(stderr, "send bytes %d : %d bytes", (int) ep_out, (int) size);
    dump_buf(bytes, size);
  }

  {
    unsigned char *buf = bytes;
    int left = size;
    size_t blksize = LUXEED_BLOCK_SIZE;

    while ( left > 0 ) {
      /* Chunk of 0x02 + 64 bytes */
      unsigned char xbuf[1 + blksize];
      int usb_result = 0;

      xbuf[0] = 0x02;
      memcpy(xbuf + 1, buf, blksize);

      int buf_size = 1 + (blksize < left ? blksize : left);

      if ( dev->opts.debug_syscall >= 3 ) {
        fprintf(stderr, "send chunk %d : %d bytes", (int) ep_out, (int) buf_size);
        dump_buf(xbuf, buf_size);
      }

      int transferred = 0;
 #if 1
      RCALL(usb_result, libusb_bulk_transfer(dev->u_dev_hd, ep_out | LIBUSB_ENDPOINT_OUT, (void*) xbuf, buf_size, &transferred, timeout));
      PDEBUG(dev, 3, "libusb_bulk_transfer(%d bytes) : transferred %d => %d", buf_size, transferred, usb_result);
#else
      RCALL(usb_result, libusb_control_transfer(dev->u_dev_hd, ep | LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, (void*) buf, buf_size, &transferred, timeout));
#endif

      if ( usb_result ) {
        luxeed_error("libusb : send failed : %d : %s", usb_result, libusb_error_name(usb_result));
#if 0
        PV(LIBUSB_ERROR_TIMEOUT, "if the transfer timed out (and populates transferred)");
        PV(LIBUSB_ERROR_PIPE, "if the endpoint halted");
        PV(LIBUSB_ERROR_OVERFLOW, "if the device offered more data, see Packets and overflows");
        PV(LIBUSB_ERROR_NO_DEVICE, "if the device has been disconnected");
        PV(LIBUSB_ERROR_BUSY, "if called from event handling context");
        PV(LIBUSB_ERROR_INVALID_PARAM, "if the transfer size is larger than the operating system and/or hardware can support (see Transfer length limitations)");
#endif
        result = -1;
        break;
      }

      if ( chunk_delay > 0 )
        usleep((int) chunk_delay * 1000000);

      if ( transferred ) {
        // int sent = blksize;
        int sent = transferred - 1;
        buf += sent; // transferred;
        left -= sent; // transferred;
      }
    }

    int ep_in = ep_out + 0x80;
    if ( 0 ) {
      int usb_result = 0;
      unsigned char buf[256];
      int buf_size = sizeof(buf);
      memcpy(buf, 0, buf_size);
      int transferred = 0;
      RCALL(usb_result, libusb_bulk_transfer(dev->u_dev_hd, ep_in, (void*) buf, buf_size, &transferred, timeout));
    }
  }

  if ( 0 ) {
    RCALL(result, libusb_clear_halt(dev->u_dev_hd, ep_out));
  }
  } while(0);

  PDEBUG(dev, 2, "(%p, %d, %p, %d) => %d", dev, ep_out, bytes, size, result);

  // if ( result ) abort();

  return result;
}


int luxeed_device_msg_checksum(luxeed_device *dev, unsigned char *buf, int size)
{
  int sum = 0;
  int i;
  int sum_save;
  int chksum_i = 0x14e; // 0x154 in total msg output.

  PDEBUG(dev, 3, "(%p, %p, %d)", dev, buf, size);

  // return 0;

  if ( dev->opts.debug >= 5 ) {
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
      luxeed_error("%p sum %02x, expected %02x", (void*) buf, (int) sum, (int) sum_save);
      assert(sum == sum_save);
    }
  } else {
    buf[chksum_i - 1] = 0;
  }

  PDEBUG(dev, 3, "(%p, %p, %d) => %d", dev, buf, size, sum);

  return sum;
}


/* Initialize all LEDS */
int luxeed_device_init(luxeed_device *dev)
{
  int result = -1;

  PDEBUG(dev, 3, "(%p)", dev);

  do {
    int slp = 100000;
    int i;
    if ( dev->initing ) {
      result = 0;
      break;
    }
    if ( dev->inited ) {
      result = 0;
      break;
    }

    PDEBUG(dev, 1, "dev initing");

    dev->initing = 1;
    dev->inited = 0;

    if ( dev->init_count == 0 ) {
      int n = 1;
      usleep(slp);
      for ( i = 0; i < n; ++ i ) {
        RCALL(result, luxeed_send (dev, LUXEED_USB_ENDPOINT_DATA, msg_ff, sizeof(msg_ff)));
        if ( result ) break;
        usleep(slp);

        RCALL(result, luxeed_send (dev, LUXEED_USB_ENDPOINT_DATA, msg_00, sizeof(msg_00)));
        if ( result ) break;
        usleep(slp);
      }
    }

    if ( result ) break;

    dev->initing = 0;
    dev->inited = 1;
    ++ dev->init_count;

    PDEBUG(dev, 1, "dev: inited");
  } while ( 0 );

  PDEBUG(dev, 3, "(%p) => %d", dev, result);

  return result;
}


const unsigned char *luxeed_device_pixel(luxeed_device *dev, int key)
{
  if ( key < 0 || key >= LUXEED_NUM_OF_KEYS )
    return 0;
  return &dev->key_data[key * 3];
}


const unsigned char * luxeed_device_set_key_color(luxeed_device *dev, luxeed_key *key, int r, int g, int b)
{
  if ( ! key ) return 0;
 unsigned char *p = (unsigned char *) luxeed_device_pixel(dev, key->id);
  if ( ! p ) return 0;
  if ( ! (p[0] == r && p[1] == g && p[2] == b) ) {
    p[0] = r;
    p[1] = g;
    p[2] = b;
    dev->key_data_dirty = 1;
  }
  return p;
}


int luxeed_device_set_key_color_all(luxeed_device *dev, int r, int g, int b)
{
  for ( int i = 0; i < LUXEED_NUM_OF_KEYS; ++ i ) {
    luxeed_key *key = luxeed_key_by_id(i);
    luxeed_device_set_key_color(dev, key, r, g, b);
  }
  return 0;
}


int luxeed_device_key_color(luxeed_device *dev, luxeed_key *key, int *r, int *g, int *b)
{
  if ( ! key ) return -1;
  const unsigned char *p = (const unsigned char *) luxeed_device_pixel(dev, key->id);
  if ( ! p ) return -1;
  *r = p[0];
  *g = p[1];
  *b = p[2];
  return 0;
}


int luxeed_device_update(luxeed_device *dev, int force)
{
  int result = 0;

  PDEBUG(dev, 3, "(%p, %d)", dev, force);

  do {
    struct timeval now;

    if ( luxeed_device_open(dev) < 0 ) {
      result = -1;
      break;
    }

    if ( luxeed_device_init(dev) < 0 ) {
      luxeed_device_close(dev);
      result = -1;
      break;
    }

    /* Only send if key data is dirty. */
    if ( ! (force || dev->key_data_dirty) ) {
      result = 0;
      PDEBUG(dev, 3, "(%p, %d): not dirty or forced", dev, force);
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
        PDEBUG(dev, 4, "  t = %d.%06d  dt = %lg secs\n", (int) now.tv_sec, (int) now.tv_usec, (double) dt);
        if ( min_frame_interval > dt ) {
          double pause_time = min_frame_interval - dt;
          PDEBUG(dev, 4, "    sleeping for %lg secs\n", (double) pause_time);
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
    PDEBUG(dev, 1, "reopen later");
    luxeed_device_close(dev);
    usleep(100000);
    return 0;
  }

  PDEBUG(dev, 3, "(%p, %d) => %d", dev, force, result);

  return result;
}

/* EOF */
