
#include "luxeedd.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static const char *error_action;
static const char *progname;

#define PDEBUG(x, ARGS ...)					    \
  do {								    \
    if ( (x)->debug ) {						    \
      fprintf(stderr, "%s: DEBUG: %s", progname, __FUNCTION__);	    \
      fprintf(stderr, ARGS);					    \
      fprintf(stderr, "\n");					    \
    }								    \
  } while (0)


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
    *(s ++) = '\0';
  }

  while ( *s && isspace(*s) )
    ++ s;

  *buf = s;
  
  return b;
}


int luxeed_read_command(luxeedd_client *cli)
{
  int result = 0;
  char buf[2048];
  size_t buf_size = sizeof(buf);
  int key_id = 0;
  unsigned char *pixel = 0;
  char *error = 0;

  PDEBUG(cli, "(%p)", cli);

  memset(buf, 0, buf_size);
  if ( fgets(buf, buf_size - 1, cli->in) == 0 ) {
    return -1;
  } else {
    char out_buf[1024];
    size_t out_buf_size = sizeof(out_buf);
    char *s = buf;
    char *cmd = parse_word(&s);
    char *word = 0;
    error = 0;

    if ( cli->debug ) {
      fprintf(stderr, "read command from %d: %s %s\n", cli->in_fd, cmd, s);
    }

    memset(out_buf, 0, sizeof(out_buf));

    if ( 0 ) {
      fprintf(stderr, "cmd = %p '%s'\n", cmd, cmd);
      fprintf(stderr, "cmd = '%c'\n", cmd && cmd[0]);
    }

    /* Blank line? */
    if ( ! cmd ) {
      return 0;
    }

    switch ( cmd[0] ) {
    case '\0':
      break;

    case 'h':
      if ( cli->out ) {
	fprintf(cli->out, "\
help: \n\
g <key-id>     : get key's current color. \n\
s r g b <key-id> ... : set key(s) to current color. \n\
u              : update keyboard colors. \n\
w <n>          : wait for n microseconds. \n\
\n\
");
      }
      break;

    case 's': // set r g b key_id ...
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

      while ( word = parse_word(&s) ) {
	key_id = -1;
	sscanf(word, "%d", &key_id);
	if ( (pixel = cli->srv->dev ? luxeed_pixel(cli->srv->dev, key_id) : 0) ) {
	  pixel[0] = cli->color[0];
	  pixel[1] = cli->color[1];
	  pixel[2] = cli->color[2];
	  snprintf(out_buf, out_buf_size, "%s %x %x %x %d", cmd, pixel[0], pixel[1], pixel[2], key_id);
	} else {
	  error = "BAD KEY";
	}
      }
      break;

    case 'g': // get key_id
      sscanf(buf, "%d", &key_id);
      if ( (pixel = cli->srv->dev ? luxeed_pixel(cli->srv->dev, key_id) : 0) ) {
	snprintf(out_buf, out_buf_size, "%s %x %x %x %d", cmd, pixel[0], pixel[1], pixel[2], key_id);
      } else {
	error = "BAD KEY";
      }
      break;

    case 'u': // update
      if ( cli->srv->dev ) {
	if ( luxeed_update(cli->srv->dev) ) {
	  error = "UPDATE FAILED";
	}
      }
      break;

    case 'w': // wait
      {
	int wait = 1000000;
	sscanf(s, "%d", &wait);
	usleep(wait);
      }
      break;

    default:
      error = "BAD COMMAND";
      break;
    }

    if ( cli->out ) {
      if ( error ) {
	fprintf(cli->out, "ERROR %s %s\n", cmd, error);
      } else {
	if ( cli->options & 0x01 ) {
	  if ( out_buf[0] ) {
	    fprintf(cli->out, "OK %s\n", out_buf);
	  } else {
	    fprintf(cli->out, "OK %s\n", cmd);
	  }
	}
      }
      
      fflush(cli->out);
    }
  }

  return 0;
}


/********************************************************************/


void luxeedd_client_read(int fd, short event, void *data)
{
  luxeedd_client *cli = data;
  int result;

  PDEBUG(cli, "(%d, %d, %p)", (int) fd, (int) event, data);

  result = luxeed_read_command(cli);

  if ( result < 0 ) {
    luxeedd_client_close(cli);
    free(cli);
  } 
  else if ( result == 0 ) {
    event_add(&cli->read_ev, 0);
  }
}



int luxeedd_client_open(luxeedd_client *cli, luxeedd_server *srv, int in_fd, int out_fd)
{
  int result = 0;

  do {
    PDEBUG(cli, "(%p, %p, %d, %d)", cli, srv, (int) in_fd, (int) out_fd);

    cli->srv = srv;
    cli->in_fd = in_fd;
    cli->out_fd = out_fd;
    cli->in = fdopen(cli->in_fd, "r");
    if ( cli->out_fd >= 0 ) {
      cli->out = fdopen(cli->out_fd, "w");
    }
    cli->options = srv->options;
    cli->debug = srv->debug;
    cli->color[0] = cli->color[1] = cli->color[2] = 0;
    
    /* Start reading on cli socket. */
    event_set(&cli->read_ev, cli->in_fd, EV_READ, &luxeedd_client_read, cli); 
    event_add(&cli->read_ev, 0);
  } while ( 0 );

  return result;
}


int luxeedd_client_close(luxeedd_client *cli)
{
  int result = 0;

  PDEBUG(cli, "(%p)", cli);

  if ( ! cli ) return 0;

  event_del(&cli->read_ev);

  if ( cli->in )
    fclose(cli->in);
  cli->in = 0;

  if ( cli->out )
    fclose(cli->out);
  cli->out = 0;

  if ( cli->in_fd >= 0) 
    close(cli->in_fd);
  cli->in_fd = -1;

  if ( cli->out_fd >= 0 )
    close(cli->out_fd);
  cli->out_fd = -1;

  return result;
}


/********************************************************************/


void luxeedd_server_accept(int fd, short event, void *data)
{
  int result = 0;
  luxeedd_server *srv = data;
  luxeedd_client *cli = 0;

  PDEBUG(srv, "(%d, %d, %p)", fd, event, data);

  do {
    int client_fd = -1;

    /* Accept another */
    event_add(&srv->accept_ev, 0);
    
    /* Open client. */
    cli = malloc(sizeof(*cli));
    memset(cli, 0, sizeof(*cli));

    /* Accept this client. */
    cli->socket_addr.sun_family = AF_UNIX;
    cli->socket_addr_size = sizeof(cli->socket_addr);
    if ( (client_fd = accept(srv->server_fd, 
			     (struct sockaddr *) &cli->socket_addr,
			     &cli->socket_addr_size)
	  ) < 0 ) {
      perror(error_action = "accept()");
      result = -1;
      break;
    }
    
    /* Do not linger. */
    {
      struct linger l = { 0, 0 };
      if ( setsockopt(client_fd, SOCK_STREAM, SO_LINGER, &l, sizeof(l)) < 0 ) {
	perror(error_action = "setsockopt: SO_LINGER");
      }
    }

    luxeedd_client_open(cli, srv, client_fd, client_fd);
  } while ( 0 );

  if ( result ) {
    luxeedd_client_close(cli);
  }
}


/********************************************************************/

int luxeedd_server_open_socket(luxeedd_server *srv)
{
  int result = 0;

  PDEBUG(srv, "(%p)", srv);

  do {
    /* Create bind address. */
    srv->socket_addr.sun_family = AF_UNIX;
    strncpy(srv->socket_addr.sun_path, srv->server_path, sizeof(srv->socket_addr.sun_path));
    srv->socket_addr_size = sizeof(srv->socket_addr);

    /* Open socket. */
    if ( (srv->server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0 ) {
      perror(error_action = "socket");
      result = -1;
      break;
    }
    
    /* Reuse address. */
#ifdef SO_REUSEADDR
    {
      int option = 1;

      if ( setsockopt(srv->server_fd, SOCK_STREAM, SO_REUSEADDR, &option, sizeof(option)) < 0 ) {
	perror(error_action = "setsockopt: SO_REUSEADDR");
      }
    }
#endif

    /* Do not linger. */
    {
      struct linger l = { 0, 0 };
      if ( setsockopt(srv->server_fd, SOCK_STREAM, SO_LINGER, &l, sizeof(l)) < 0 ) {
	perror(error_action = "setsockopt: SO_LINGER");
      }
    }

    /* Bind address. */
    if ( bind(srv->server_fd, 
	      (const struct sockaddr *) &srv->socket_addr, 
	      (socklen_t) srv->socket_addr_size
	      ) < 0 ) {
      perror(error_action = "bind");
      result = -1;
      break;
    }

    /* Set perms/ownership. */
    if ( srv->socket_addr.sun_family == AF_UNIX ) {
      if ( fchmod(srv->server_fd, 0644) < 0 ) {
	perror(error_action = "fchmod(srv->server_fd, ...)");
	// result = -1;
	// break;
      }
    }

    /* Listen. */
    if ( listen(srv->server_fd, 5) < 0 ) {
      perror(error_action = "listen");
      result = -1;
      break;
    }

    /* Register for accept events. */
    event_set(&srv->accept_ev, srv->server_fd, EV_READ, &luxeedd_server_accept, srv); 
    event_add(&srv->accept_ev, 0);

  } while ( 0 );

  return result;
}


int luxeedd_server_open_fifo(luxeedd_server *srv)
{
  int result = 0;

  PDEBUG(srv, "(%p)", srv);

  do {
    /* Delete old file. */
    {
      struct stat s;
      if ( stat(srv->server_path, &s) == 0 ) {
	if ( unlink(srv->server_path) < 0 ) {
	  perror(error_action = "unlink");
	  // result = -1;
	  // break;
	}
      }
    }

    /* Create a named pipe (fifo). */
    if ( mknod(srv->server_path, S_IFIFO | 0666, 0) < 0 ) {
      perror(error_action = "mknod");
      // result = -1;
      // break;
    }

    do {
      luxeedd_client *cli;
      int client_fd;

      PDEBUG(srv, ": open(%s) ...", srv->server_path);
      
      if ( (client_fd = open(srv->server_path, O_RDONLY)) < 0 ) {
	perror(error_action = "open");
	result = -1;
	break;
      }

      PDEBUG(srv, ": open(%s) => %d", srv->server_path, client_fd);

      /* Open client. */
      cli = malloc(sizeof(*cli));
      memset(cli, 0, sizeof(*cli));

      luxeedd_client_open(cli, srv, client_fd, -1);

      /* Process events. */
      event_dispatch();
    } while ( 1 );
  } while ( 0 );

  return result;
}

int luxeedd_server_open(luxeedd_server *srv)
{
  int result = 0;

  PDEBUG(srv, "(%p)", srv);

  do {
    srv->server_fd = -1;
    
    // strcpy(srv->server_uri, "tcp://127.0.0.1:25324");
    strncpy(srv->server_path, "/tmp/luxeed", sizeof(srv->server_path));

    /* Find the luxeed device. */
    if ( ! (srv->dev = luxeed_find_device(0, 0)) ) {
      fprintf(stderr, "%s: luxeed keyboard not found\n", progname);
      error_action = "luxeed_find_device";
      // result = -1;
      // break;
    }

    /* Attempt to open the device now. */
    if ( srv->dev ) {
      if ( (result = luxeed_open(srv->dev)) ) {
	error_action = "luxeed_open";
	// result = -1;
	// break;
      }
    }

#if 0
    if ( luxeeed_server_open_socket(srv) ) {
      error_action = "luxeed_open";
      result = -1;
      break;
    }
#endif

    if ( luxeedd_server_open_fifo(srv) ) {
      error_action = "luxeed_open";
      result = -1;
      break;
    }

  } while ( 0 );

  if ( result ) {
    luxeedd_server_close(srv);
  }

  return result;
}


int luxeedd_server_close(luxeedd_server *srv)
{
  int result = 0;

  if ( ! srv ) return 0;

  do {
    event_del(&srv->accept_ev);

    if ( srv->dev ) {
      luxeed_destroy(srv->dev);
    }
    if ( srv->server_fd >= 0 ) {
      close(srv->server_fd);
    }
    unlink(srv->server_path);
  } while ( 0 );

  return result;
}


/********************************************************************/


int main (int argc, char **argv)
{
  int result = 0;
  luxeedd_server _srv, *srv = &_srv;
 
  progname = argv[0];
  
  do {
    /* Initialize libevent. */
    event_init();
    
    memset(srv, 0, sizeof(*srv));
    srv->debug = 1;
    if ( argc > 1 ) {
      srv->options |= 1;
    }
    srv->debug = 1;

    /* Open server and device. */
    result = luxeedd_server_open(srv);
    if ( result ) break;

    /* Process events. */
    event_dispatch();

  } while ( 0 );

  if ( result ) {
    fprintf(stderr, "%s: error: %d\n", progname, result);
    perror(error_action ? error_action : progname);
  }

  /* Close server and device. */
  luxeedd_server_close(srv);

  return result != 0 ? 1 : 0;
}


/* EOF */

