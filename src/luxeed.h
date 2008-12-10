#ifndef LUXEED_H
#define LUXEED_H

#include <usb.h>

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

#define LUXEED_NUM_OF_KEYS 76
  /* RGB order, 0x00 - 0xff scale. */
  unsigned char key_data[LUXEED_NUM_OF_KEYS * 3];

  /* Message buffer */
  unsigned char *msg;
  int msg_len;
  int msg_size;

  int debug;
} luxeed_device;

typedef struct luxeed_key {
  int id;
  const char *name[3];
  int code[3];
} luxeed_key;

luxeed_device *luxeed_find_device(uint16_t vendor, uint16_t product);
int luxeed_open(luxeed_device *dev);
int luxeed_close(luxeed_device *dev);
int luxeed_destroy(luxeed_device *dev);
int luxeed_init(luxeed_device *dev);
int luxeed_send(luxeed_device *dev, int ep, unsigned char *bytes, int size);
int luxeed_update(luxeed_device *dev);

luxeed_key *luxeed_key_id(luxeed_device *dev, int i);
luxeed_key *luxeed_key_name(luxeed_device *dev, const char *keyname);
luxeed_key *luxeed_key_ascii(luxeed_device *dev, int c);

#define LUXEED_COLOR_MIN 0x00
#define LUXEED_COLOR_MAX 0xff

unsigned char *luxeed_pixel(luxeed_device *dev, int key);
int luxeed_set_key_color(luxeed_device *dev, luxeed_key *key, int r, int g, int b);
int luxeed_key_color(luxeed_device *dev, luxeed_key *key, int *r, int *g, int *b);

#endif
