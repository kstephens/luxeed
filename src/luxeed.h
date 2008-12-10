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

  unsigned char key_data[64 * 3];

  unsigned char msg[0x180];
  int msg_size;
} luxeed_device;

luxeed_device *luxeed_find_device(uint16_t vendor, uint16_t product);
int luxeed_open(luxeed_device *dev);
int luxeed_close(luxeed_device *dev);
int luxeed_init(luxeed_device *dev);
int luxeed_send(luxeed_device *dev, int ep, unsigned char *bytes, int size);
int luxeed_update(luxeed_device *dev);

#endif
