
/*
sudo apt-get install libusb-dev
*/

#include <stdio.h>
#include <stdlib.h> /* memset(), memcpy() */
#include <string.h>
#include <usb.h>

#define LUXHEED_USB_VENDOR   0x534B
#define LUXHEED_USB_PRODUCT  0x0600
#define LUXHEED_USB_ENDPOINT_DATA 1
#define LUXHEED_USB_ENDPOINT_CONTROL 1

typedef struct luxheed_device {
  int id;

  struct usb_bus *u_bus;
  struct usb_device *u_dev;
  usb_dev_handle *u_dh;

  unsigned char key_data[64];
  unsigned char msg[256];
  int msg_size;
} luxheed_device;
static int luxheed_dev_id = 0;

static int usb_debug_level = 9;


static int luxheed_open(luxheed_device *dev)
{
  int result = -1;

  do {
    dev->u_dh = usb_open(dev->u_dev);
    if ( ! dev->u_dh ) {
      return -1;
    }
    
    result = usb_claim_interface(dev->u_dh, 1);
    // result = usb_claim_interface(dev->u_dh, 2);
    
    result = 0;

#if 0
    result = usb_set_configuration(dev->u_dh, USB_RECIP_INTERFACE);
    
    result = usb_reset(dev->u_dh);
    
    if ( ! result ) {
      abort();
    }
    
    result = usb_resetep(dev->u_dh, LUXHEED_USB_ENDPOINT_DATA);
    result = usb_resetep(dev->u_dh, LUXHEED_USB_ENDPOINT_CONTROL);
#endif
  } while ( 0 );

  return result;
}


static int luxheed_close(luxheed_device *dev)
{
  int result = -1;

  do {
    if ( dev->u_dh ) {
      usb_release_interface(dev->u_dh, 1);
      usb_release_interface(dev->u_dh, 2);
      result = usb_close(dev->u_dh);
      if ( result ) break;

      dev->u_dh = 0;
    }
  } while ( 0 );

  return result;
}


static luxheed_device *luxheed_find_device(uint16_t vendor, uint16_t product)
{
  struct usb_bus *bus;
  struct usb_device *dev;
  struct usb_bus *busses;

  if ( ! vendor ) {
    vendor  = LUXHEED_USB_VENDOR;
  }
  if ( ! product ) {
    product = LUXHEED_USB_PRODUCT;
  }

  usb_init();
  usb_find_busses();
  usb_find_devices();
  busses = usb_get_busses();

  for ( bus = busses; bus; bus = bus->next ) {
    for ( dev = bus->devices; dev; dev = dev->next ) {
      if ( (dev->descriptor.idVendor == vendor) && (dev->descriptor.idProduct == product) ) {
	luxheed_device *luxheed = malloc(sizeof(*luxheed));

	memset(luxheed, 0, sizeof(*luxheed));
	luxheed->id = luxheed_dev_id ++;
	luxheed->u_bus = bus;
	luxheed->u_dev = dev;

	fprintf(stderr, "  bus %s 0x%x\n", (char*) bus->dirname, (int) bus->location);
	fprintf(stderr, "  dev %s 0x%x\n", (char*) dev->filename, (int) dev->devnum);

	usb_set_debug(usb_debug_level);
  
        return luxheed;
      }
    }
  }

  return NULL;
}


static int luxheed_send_bytes(luxheed_device *dev, int ep, unsigned char *bytes, int size)
{
  int result = -1;

  usb_dev_handle *dh;

  if ( luxheed_open(dev) ) {
    return -1;
  }

  dh = dev->u_dh;

  fprintf(stderr, "send_bytes(%d, %d)...", (int) ep, (int) size);
  {
    int i;
    for ( i = 0; i < size; ++ i ) {
      if ( i % 0x10 == 0 ) {
	fprintf(stderr, "\n");
      }
      fprintf(stderr, "%02x ", (int) bytes[i]);
    }
    fprintf(stderr, "\n\n");
  }

  fprintf(stderr, "write ep = %d, %d ...", (int) ep + USB_ENDPOINT_OUT, (int) size);
  result = usb_bulk_write(dh, ep + USB_ENDPOINT_OUT, bytes, size, 0);
  // result = usb_interrupt_write(dh, ep, bytes, size, 0);
  fprintf(stderr, "\n");

  fprintf(stderr, "send_bytes(%d, %d) => %d\n", (int) ep, (int) size, (int) result);

  if ( result != size ) {
    abort();
  }

  /* Read result */
  {
    char buf[1024];
    int buf_size = sizeof(buf);
    fprintf(stderr, "read ep = %d, %d...", (int) ep + USB_ENDPOINT_IN, (int) size);
    result = usb_bulk_read(dh, ep + USB_ENDPOINT_IN, buf, buf_size, 0);
    fprintf(stderr, "%d bytes\n", (int) result);
    if ( result < 0 ) {
      abort();
    }
  }

  return result;
}



static int luxheed_update(luxheed_device *dev)
{
  int result = -1;

  do {
#if 0
    /* Send control */
    {
      static unsigned char ep_2[] = { 0x01, 0x079 };
      
      result = luxheed_send_bytes(dev, LUXHEED_USB_ENDPOINT_CONTROL, ep_2, sizeof(ep_2));
      if ( result ) break;
    }
#endif

    /* Send data */
    dev->msg[0] = 0x02;
    memcpy(dev->msg + 1, dev->key_data, 64);
    dev->msg_size = 65;

    result = luxheed_send_bytes(dev, LUXHEED_USB_ENDPOINT_DATA, dev->msg + 1, dev->msg_size - 1);
    if ( result ) break;

  } while ( 0 );

  return result;
}



int main (int argc, char **argv)
{
  luxheed_device *dev;

  dev = luxheed_find_device(0, 0);

  //If the keyboard wasn't found the function will have returned NULL
  if ( ! dev ) {
    fprintf(stderr, "luxheed keyboard not found\n");
    return 1;
  }

  {
    int msg_id = 0;

    while ( 1 ) {
      dev->key_data[msg_id % 64] = (0xff - msg_id) & 0xff;
      luxheed_update(dev);
      sleep(1);
    }
  }

  luxheed_close(dev);
  
  return 0;
}
