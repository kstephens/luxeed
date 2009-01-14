#ifndef LUXEED_DEVICE_H
#define LUXEED_DEVICE_H

#include <usb.h>
#include <time.h> /* struct timeval */

#define LUXEED_USB_VENDOR   0x534B
#define LUXEED_USB_PRODUCT  0x0600
#define LUXEED_USB_INTERFACE 1
#define LUXEED_USB_ENDPOINT_DATA 0x02
#define LUXEED_USB_ENDPOINT_CONTROL 0x02

typedef struct luxeed_device {
  int id;

  struct usb_bus *u_bus;
  struct usb_device *u_dev;
  usb_dev_handle *u_dh;

  short opening;
  short opened;
  short initing;
  short inited;
  int init_count;

#define LUXEED_NUM_OF_KEYS 76
  /* RGB order, 0x00 - 0xff scale. */
  unsigned char key_data[LUXEED_NUM_OF_KEYS * 3];

  /* Message buffer */
  unsigned int msg_id;
  unsigned char *msg;
  int msg_len;
  int msg_size;

  struct timeval update_last_send_time;
  
  int debug;
} luxeed_device;

typedef struct luxeed_key {
  int id; /* index into luxeed_device.key_data[* 3] */
  const char *name[3];
  int code[3];
  int shift;
} luxeed_key;


luxeed_device *luxeed_device_create();
int luxeed_device_destroy(luxeed_device *dev);
int luxeed_device_find(luxeed_device *dev, uint16_t vendor, uint16_t product);
int luxeed_device_open(luxeed_device *dev);
int luxeed_device_close(luxeed_device *dev);
int luxeed_device_init(luxeed_device *dev);
int luxeed_device_send(luxeed_device *dev, int ep, unsigned char *bytes, int size);
int luxeed_device_update(luxeed_device *dev);

int luxeed_device_msg_checksum(luxeed_device *dev, unsigned char *buf, int size);

luxeed_key *luxeed_device_key_id(luxeed_device *dev, int i);
luxeed_key *luxeed_device_key_name(luxeed_device *dev, const char *keyname);
luxeed_key *luxeed_device_key_ascii(luxeed_device *dev, int c);

#define LUXEED_COLOR_MIN 0x00
#define LUXEED_COLOR_MAX 0xff

unsigned char *luxeed_device_pixel(luxeed_device *dev, int key_id);
int luxeed_device_set_key_color(luxeed_device *dev, luxeed_key *key, int r, int g, int b);
int luxeed_device_key_color(luxeed_device *dev, luxeed_key *key, int *r, int *g, int *b);

#endif

