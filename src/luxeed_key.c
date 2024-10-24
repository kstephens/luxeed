#include "luxeed_key.h"
#include "luxeed.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

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


int luxeed_key_init()
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

  luxeed_key_init();

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


luxeed_key *luxeed_key_by_id(int id)
{
  int key_i;

  if ( id < 0 || id >= LUXEED_NUM_OF_KEYS )
    return 0;

  luxeed_key_init();

  for ( key_i = 0; key_i < LUXEED_NUM_OF_KEYS; ++ key_i )
    if ( _keys[key_i].id == id )
      return &_keys[key_i];

  return 0;
}


luxeed_key *luxeed_key_by_position(int x, int y)
{
  if ( x < 0 || y < 0 )
    return 0;

  luxeed_key_init();

  int key_i;
  for ( key_i = 0; key_i < LUXEED_NUM_OF_KEYS; ++ key_i ) {
    luxeed_key *k = &_keys[key_i];
    if ( k->x == x && k->y == y )
      return k;
  }

  return 0;
}


luxeed_key *luxeed_key_by_ascii(int c)
{
  int key_i;

  if ( ! (0 <= c && c <= 127) )
    return 0;

  luxeed_key_init();

  for ( key_i = 0; key_i < LUXEED_NUM_OF_KEYS; ++ key_i ) {
    int j;
    luxeed_key *k = &_keys[key_i];
    int code;
    for ( j = 0; (code = k->code[j]); ++ j )
      if ( code == c )
        return k;
  }

  return 0;
}


luxeed_key *luxeed_key_by_name(const char *keyname)
{
  int key_i;

  if ( ! keyname || ! *keyname )
    return 0;

  luxeed_key_init();

  for ( key_i = 0; key_i < LUXEED_NUM_OF_KEYS; ++ key_i ) {
    int j;
    const char *kn;
    for ( j = 0; (kn = _keys[key_i].name[j]); ++ j ) {
      if ( ! strcmp(kn, keyname) )
        return &_keys[key_i];
    }
  }
  return 0;
}


luxeed_key *luxeed_key_by_string(const char *str)
{
  if ( ! str || ! *str )
    return 0;

  luxeed_key_init();

  if ( *str == '#' && str[1] ) {
    return luxeed_key_by_id(atoi(str + 1));
  }
  else if ( *str == '@' && str[1] ) {
    int x = -1, y = -1;
    sscanf(str + 1, "%d,%d", &x, &y);
    return luxeed_key_by_position(x, y);
  }
  else if ( *str == '0' && str[1] == 'x' && str[2] ) {
    int code = -1;
    sscanf(str + 2, "%x", &code);
    return luxeed_key_by_ascii(code);
  }
  else {
    return luxeed_key_by_name(str);
  }
}
