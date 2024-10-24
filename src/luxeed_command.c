
#include "luxeed_command.h"
#include <ctype.h>


/********************************************************************/


static char *parse_word(char **buf)
{
  char *s = *buf;
  char *b = 0;

  if ( ! s )
    return 0;

  while ( *s && isspace(*s) )
    ++ s;

  if ( *s ) {
    b = s;

    while ( *s && ! isspace(*s) )
      ++ s;

    if ( *s ) {
      *(s ++) = '\0';

      while ( *s && isspace(*s) )
	++ s;
    }
  }

  *buf = s;

  return b;
}


/********************************************************************/


int luxeed_client_run_command(luxeed_client *cli, char *buf, char *out_buf, size_t out_buf_size, int *force_outputp, char **cmdp, char **errorp, char **error2p)
{
  int result = 0;
  const unsigned char *pixel = 0;
#define force_output (*force_outputp)
#define cmd (*cmdp)
#define error (*errorp)
#define error2 (*error2p)
  char *s = 0;
  char *word = 0;

  PDEBUG(cli, 3, "(%p)", cli);

  force_output = 0;
  cmd = "<<UNKNOWN>>";
  error = 0;
  error2 = "";

  /* Clear output buffer. */
  memset(out_buf, 0, out_buf_size);

  /* Start at beginning of buffer */
  s = buf;

  /* Parse command as first word */
  cmd = parse_word(&s);

  if ( cli->opts.debug >= 3 ) {
    fprintf(stderr, "  cmd = %p \"%s\", s = \"%s\" \n", cmd, cmd ? cmd : "<<NULL>>", s ? s : "<<NULL>");
  }

  /* Blank line? */
  if ( ! cmd ) {
    return 0;
  }

  switch ( cmd[0] ) {
  case '\0': case '#':
    break;

  case 'h':
    strcpy(out_buf, "\
help: \n\
g <key-id>     : get key's current color. \n\
s r g b <key-id> ... : set key(s) to current color. \n\
u              : update keyboard colors. \n\
w <sec>        : wait for sec seconds. \n\
d <level>      : set debug level \n\
D <level>      : set global debug level \n\
\n\
");
    break;

  case 'v': /* verbose */
    if ( (word = parse_word(&s)) ) {
      cli->opts.verbose = atoi(word);
    }
    break;

  case 'd': /* debug */
    if ( (word = parse_word(&s)) ) {
      int val = atoi(word);
      cli->opts.debug = val;
    }
    break;

  case 'D': /* debug */
    if ( (word = parse_word(&s)) ) {
      int val = atoi(word);

      cli->opts.debug =
      cli->srv->opts.debug =
      val;
      if ( cli->srv->dev ) {
        cli->srv->dev->opts.debug = val;
      }
    }
    break;

  case 's': /* set r g b key_id ... */
    /* r */
    if ( (word = parse_word(&s)) ) {
      sscanf(word, "%2x", &cli->color[0]);
    }
    /* g */
    if ( (word = parse_word(&s)) ) {
      sscanf(word, "%2x", &cli->color[1]);
    }
    /* b */
    if ( (word = parse_word(&s)) ) {
      sscanf(word, "%2x", &cli->color[2]);
    }

    while ( (word = parse_word(&s)) ) {
      if ( strcmp(word, "ALL") == 0 ) {
        luxeed_device_set_key_color_all(cli->srv->dev, cli->color[0], cli->color[1], cli->color[2]);
      } else {
        luxeed_key *key = luxeed_device_key_by_string(cli->srv->dev, word);
        if ( key && (pixel = luxeed_device_set_key_color(cli->srv->dev, key, cli->color[0], cli->color[1], cli->color[2])) ) {
          if ( cli->opts.debug >= 4 ) {
            fprintf(stderr, "  word %-10s => key->id = %3d, key->name[0] = %s\n", word, key->id, key->name[0]);
          }
          snprintf(out_buf, out_buf_size, "%s %x %x %x #%d", cmd, pixel[0], pixel[1], pixel[2], key->id);
        } else {
          error = "BAD KEY";
          error2 = word;
        }
      }
    }
    break;

  case 'g': /* get key_id */
    while ( (word = parse_word(&s)) ) {
      luxeed_key *key = luxeed_device_key_by_string(cli->srv->dev, word);
      if ( key && (pixel = cli->srv->dev ? luxeed_device_pixel(cli->srv->dev, key->id) : 0) ) {
        if ( cli->opts.debug >= 4 ) {
          fprintf(stderr, "  word %-10s => key->id = %3d, key->name[0] = %s\n", word, key->id, key->name[0]);
        }
        snprintf(out_buf, out_buf_size, "%s %x %x %x #%d", cmd, pixel[0], pixel[1], pixel[2], key->id);
        force_output = 1;
      } else {
        error = "BAD KEY";
        error2 = word;
      }
    }
    break;

  case 'u': /* update */
    if ( cli->srv->dev ) {
      /* Do not force update. */
      if ( luxeed_device_update(cli->srv->dev, 0) ) {
        error = "UPDATE FAILED";
      }
    }
    break;

  case 'w': /* wait */
    {
      double wait = 1.0;
      sscanf(s, "%lg", &wait);
      luxeed_client_sleep(cli, wait);
      result = 1;  /* Prevent caller from enabling read event. */
    }
    break;

  default:
    error = "BAD COMMAND";
    break;
  }

  PDEBUG(cli, 3, "(%p) => %d", cli, result);

  return result;

#undef force_output
#undef cmd
#undef error
#undef error2
}



/* EOF */
