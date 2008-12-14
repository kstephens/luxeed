
#include "luxeedd.h"
#include "event.h"


static char *progname;

/********************************************************************/

static int luxeedd_client_read(int fd, short event, void *data)
{
  luxeedd_client *cli = data;
  int result = -1;

  result = luxeed_read_command(cli);

  if ( result < 0 ) {
    fclose(cli->in);
    fclose(cli->out);
    close(cli->in_fd);
    close(cli->out_fd);
    free(cli);
  } 
  else if ( result == 0 ) {
    event_add(&cli->read_ev, 0);
  }

  return 0;
}


/********************************************************************/


static struct event luxeedd_client_accept_ev;
static int luxeedd_client_accept(int fd, short event, void *data)
{
  luxeedd_client *cli;

  cli = malloc(sizeof(*cli));
  memset(cli, 0, sizeof(*cli));
  
  cli->dev = dev;
  cli->in_fd = fd;
  cli->out_fd = fd;
  cli->in =  fdopen(cli->in_fd, "r");
  cli->out = fdopen(cli->out_fd, "w");
  cli->options = 0;
  cli->color[0] = cli->color[1] = cli->color[2] = 0;

  event_set(&cli->read_ev, cli->in_fd, EV_READ, &luxeedd_read_command, cli); 
  event_add(&cli->read_ev, 0);

  return 0;
}


/********************************************************************/


const char *socket_path = "/tmp/luxeed";

int main (int argc, char **argv)
{
  int result = 0;
  luxeed_device *dev = 0;
  int verbose = 0;

  progname = argv[0];
  
  if ( argc > 1 ) {
    verbose = 1;
  }

  dev = luxeed_find_device(0, 0);

  if ( ! dev ) {
    fprintf(stderr, "%s: luxeed keyboard not found\n", progname);
    return 1;
  }

  do {
    int socket_fd = 0;

    result = luxeed_open(dev);
    if ( result ) break;
    
    mknod(socket_path);
    
    event_init();
    
    event_set(&luxeedd_client_accept_ev, socket_fd, EV_READ, &luxeedd_client_accept, 0); 
    event_add(&luxeedd_client_accept_ev, 0);

    event_dispatch();

  } while ( 0 );

 done:
  if ( result ) {
    fprintf(stderr, "error: %d\n", result);
  }
  luxeed_destroy(dev);
  
  return 0;
}


/* EOF */

