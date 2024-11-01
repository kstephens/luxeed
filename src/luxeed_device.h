#ifndef LUXEED_DEVICE_H
#define LUXEED_DEVICE_H

#include <stdio.h>
#include "luxeed.h" /* luxeed_options */
#include "luxeed_key.h"

#ifdef __APPLE__
typedef unsigned short uint16_t;
#endif

#define LUXEED_USB_VENDOR   0x534B
#define LUXEED_USB_PRODUCT  0x0600
#define LUXEED_USB_INTERFACE 1
#define LUXEED_USB_ENDPOINT_DATA 0x02
#define LUXEED_USB_ENDPOINT_CONTROL 0x82

typedef struct luxeed_device {
  int id;
  struct luxeed_options opts;

  libusb_context* u_ctx;
  libusb_device* u_dev;
  struct libusb_device_descriptor u_dev_desc;
  libusb_device_handle* u_dev_hd;

  short opening;
  short opened;
  short initing;
  short inited;
  int init_count;

  /* RGB order, 0x00 - 0xff scale. */
  unsigned char key_data[LUXEED_NUM_OF_KEYS * 3];
  int key_data_dirty;
  struct timeval update_last_send_time;

  /* Message buffer */
  unsigned int msg_id;
  unsigned char *msg;
  int msg_len;
  int msg_size;

} luxeed_device;

luxeed_device *luxeed_device_create();
int luxeed_device_destroy(luxeed_device *dev);
int luxeed_device_open(luxeed_device *dev);
int luxeed_device_close(luxeed_device *dev);
int luxeed_device_opened(luxeed_device *dev);
int luxeed_device_init(luxeed_device *dev);
int luxeed_device_send(luxeed_device *dev, int ep, unsigned char *bytes, int size);
int luxeed_device_update(luxeed_device *dev, int force);

int luxeed_device_msg_checksum(luxeed_device *dev, unsigned char *buf, int size);

void luxeed_key_map_dump(FILE *out);

#define LUXEED_COLOR_MIN 0x00
#define LUXEED_COLOR_MAX 0xff

const unsigned char *luxeed_device_pixel(luxeed_device *dev, int key_id);
const unsigned char *luxeed_device_set_key_color(luxeed_device *dev, luxeed_key *key, int r, int g, int b);
int luxeed_device_key_color(luxeed_device *dev, luxeed_key *key, int *r, int *g, int *b);

int luxeed_device_set_key_color_all(luxeed_device *dev, int r, int g, int b);

#endif
