#ifndef LUXEED_KEY_H
#define LUXEED_KEY_H 1

#include "luxeed_device.h"

int luxeed_init_keys();
void luxeed_key_map_dump(FILE *out);
luxeed_key *luxeed_device_key_by_id(luxeed_device *dev, int id);
luxeed_key *luxeed_device_key_by_position(luxeed_device *dev, int x, int y);
luxeed_key *luxeed_device_key_by_ascii(luxeed_device *dev, int c);
luxeed_key *luxeed_device_key_by_name(luxeed_device *dev, const char *keyname);
luxeed_key *luxeed_device_key_by_string(luxeed_device *dev, const char *str);

#endif
