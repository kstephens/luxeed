#ifndef LUXEED_KEY_H
#define LUXEED_KEY_H 1
#include <stdio.h>

#define LUXEED_NUM_OF_KEYS 76

typedef struct luxeed_key {
  int id;               /* Index into luxeed_device.key_data[* 3]. */
  int mapped;           /* Non-zero if key is actually mapped to an LED. */
  const char *name[3];  /* The key name. */
  int code[3];          /* The ASCII code for this key-press. */
  int shift[3];         /* Non-zero if ASCII code is composed with SHIFT and key. */
  int x, y;             /* Position relative to the top-leftmost key (~). */
} luxeed_key;


int luxeed_key_init();
void luxeed_key_map_dump(FILE *out);
luxeed_key *luxeed_key_by_id(int id);
luxeed_key *luxeed_key_by_position(int x, int y);
luxeed_key *luxeed_key_by_ascii(int c);
luxeed_key *luxeed_key_by_name(const char *keyname);
luxeed_key *luxeed_key_by_string(const char *str);

#endif
