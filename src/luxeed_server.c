
#include "luxeed.h"
#include "luxeed_server.h"
#include "luxeed_endpoint.h"
#include "luxeed_command.h"
#include <ctype.h>


/********************************************************************/


/********************************************************************/

int luxeed_client_read_command(luxeed_client *cli)
{
  int result = 0;
  char buf[2048];
  size_t buf_size = sizeof(buf);

  PDEBUG(cli, 4, "(%p)", cli);

  memset(buf, 0, buf_size);
  if ( luxeed_endpoint_read_line(&cli->ep, buf, buf_size) < 0 ) {
    result = -1;
  } else {
    char out_buf[1024];
    size_t out_buf_size = sizeof(out_buf);
    int force_output = 0;
    char *cmd = "<<UNKNOWN>>";
    char *error = 0;
    char *error2 = "";

    if ( cli->opts.debug >= 5 ) {
      fprintf(stderr, "read command from %d:\n", cli->ep.in_fd);
    }

    result = luxeed_client_run_command(cli, buf, out_buf, out_buf_size, &force_output, &cmd, &error, &error2);

    if ( cli->ep.out ) {
      if ( error ) {
	fprintf(cli->ep.out, "ERROR %s %s %s\n", cmd, error, error2);
      } else {
	if ( cli->opts.verbose || force_output ) {
	  if ( out_buf[0] ) {
	    fprintf(cli->ep.out, "OK %s\n", out_buf);
	  } else {
	    fprintf(cli->ep.out, "OK %s\n", cmd);
	  }
	}
      }
      
      fflush(cli->ep.out);
    }
  }

  PDEBUG(cli, 4, "(%p) => %d", cli, result);

  return result;
}



int luxeed_client_sleep(luxeed_client *cli, double sec)
{
  int result = 0;

  PDEBUG(cli, 3, "(%p, %g)", cli, (double) sec);

  if ( sec < 0 ) {
    return result;
  }

  /* Stop reading from client. */
  if ( cli->ep.read_ev_active ) {
    cli->ep.read_ev_active = 0;
    event_del(&cli->ep.read_ev);
  }

  /* Add the timer. */
  cli->ep.timer_timeval.tv_sec = (time_t) sec;
  cli->ep.timer_timeval.tv_usec = (long) ((sec - (time_t) sec) * 1000000.0);
  cli->ep.timer_ev_active = 1;
  evtimer_add(&cli->ep.timer_ev, &cli->ep.timer_timeval);

  PDEBUG(cli, 3, "(%p, %g) => %d", cli, (double) sec, (int) result);

  return result;
}


void luxeed_client_sleep_finished(int fd, short event, void *data)
{
  luxeed_client *cli = data;
  int result = 0;

  PDEBUG(cli, 3, "(%d, %d, %p)", (int) fd, (int) event, data);

  /* Remove the timer. */
  if ( cli->ep.read_ev_active ) {
    cli->ep.timer_ev_active = 0;
    evtimer_del(&cli->ep.timer_ev);
  }

  /* Continue reading from client. */
  cli->ep.read_ev_active = 1;
  event_add(&cli->ep.read_ev, 0);

  PDEBUG(cli, 3, "(%d, %d, %p) => %d", (int) fd, (int) event, data, result);
}


/********************************************************************/


void luxeed_client_read(int fd, short event, void *data)
{
  luxeed_client *cli = data;
  int result = 0;

  PDEBUG(cli, 4, "(%d, %d, %p)", (int) fd, (int) event, data);

  result = luxeed_client_read_command(cli);

  if ( result < 0 ) {
    luxeed_client_close(cli);
    free(cli);
  } 
  else if ( result == 0 ) {
    cli->ep.read_ev_active = 1;
    event_add(&cli->ep.read_ev, 0);
  }
  else {
    /* I/O read is sleeping after wait command: see luxeed_command.c */
  }

  PDEBUG(cli, 4, "(%d, %d, %p) => %d", (int) fd, (int) event, data, result);
}


int luxeed_client_open(luxeed_client *cli, luxeed_server *srv, int in_fd, int out_fd)
{
  int result = 0;

  do {
    PDEBUG(cli, 2, "(%p, %p, %d, %d)", cli, srv, (int) in_fd, (int) out_fd);

    cli->srv = srv;
    cli->opts = srv->opts;
    cli->ep.opts = cli->opts;

    luxeed_endpoint_open(&cli->ep, in_fd, out_fd);

    cli->color[0] = cli->color[1] = cli->color[2] = 0;
    
    /* Start reading on cli socket. */
    cli->ep.read_ev_active = 1;
    event_set(&cli->ep.read_ev, cli->ep.in_fd, EV_READ, &luxeed_client_read, cli); 
    event_add(&cli->ep.read_ev, 0);

    /* Prepare a timer for "wait" commands. */
    cli->ep.timer_ev_active = 1;
    evtimer_set(&cli->ep.timer_ev, luxeed_client_sleep_finished, (void *) cli);
  
  } while ( 0 );

  PDEBUG(cli, 2, "(%p, %p, %d, %d) => %d", cli, srv, (int) in_fd, (int) out_fd, (int) result);

  return result;
}


int luxeed_client_close(luxeed_client *cli)
{
  int result = 0;

  PDEBUG(cli, 2, "(%p)", cli);

  if ( ! cli ) return 0;

  /* Remove timer event. */
  if ( cli->ep.timer_ev_active ) {
    cli->ep.timer_ev_active = 0;
    evtimer_del(&cli->ep.timer_ev);
  }
  
  /* Stop read events. */
  if ( cli->ep.read_ev_active ) {
    cli->ep.read_ev_active = 0;
    event_del(&cli->ep.read_ev);
  }

  /* Close FDs and FILEs. */
  luxeed_endpoint_close(&cli->ep);

  /* Update the device, if open and dirty. */
  if ( luxeed_device_opened(cli->srv->dev) ) {
    luxeed_device_update(cli->srv->dev, 0);
  }

  PDEBUG(cli, 2, "(%p) => %d", cli, result);

  return result;
}



/********************************************************************/


void luxeed_server_accept(int fd, short event, void *data)
{
  int result = 0;
  luxeed_server *srv = data;
  luxeed_client *cli = 0;

  PDEBUG(srv, 2, "(%d, %d, %p)", fd, event, data);

  do {
    /* Accept another. */
    srv->ep.accept_ev_active = 1;
    event_add(&srv->ep.accept_ev, 0);
    
    /* Open client. */
    cli = malloc(sizeof(*cli));
    memset(cli, 0, sizeof(*cli));

    /* Accept client */
    if ( luxeed_endpoint_accept(&srv->ep, &cli->ep) < 0 ) {
      perror(luxeed_error_action = "luxeed_endpoint_accept()");
      result = -1;
      break;
    }

    luxeed_client_open(cli, srv, cli->ep.in_fd, cli->ep.in_fd);
  } while ( 0 );

  if ( result ) {
    luxeed_client_close(cli);
  }

  PDEBUG(srv, 2, "(%p) => %d", srv, result);

}


/********************************************************************/


int luxeed_server_open_socket(luxeed_server *srv)
{
  int result = 0;

  PDEBUG(srv, 1, "(%p)", srv);

  do {
    if ( (result = luxeed_endpoint_bind(&srv->ep)) ) {
      perror(luxeed_error_action = "luxeed_endpoint_bind");
      break;
    }

    /* Register for accept events. */
    srv->ep.accept_ev_active = 1;
    event_set(&srv->ep.accept_ev, srv->ep.in_fd, EV_READ, &luxeed_server_accept, srv); 
    event_add(&srv->ep.accept_ev, 0);
  } while ( 0 );

  PDEBUG(srv, 1, "(%p) => %d, fd = %d", srv, result, srv->ep.in_fd);

  return result;
}


int luxeed_server_open_inet(luxeed_server *srv)
{
  int result = 0;

  PDEBUG(srv, 1, "(%p)", srv);

  do {
    /* Create socket address. */
    srv->ep.socket_family = AF_INET;

    result = luxeed_server_open_socket(srv);
  } while ( 0 );

  PDEBUG(srv, 1, "(%p) => %d", srv, result);

  return result;
}


int luxeed_server_open_uds(luxeed_server *srv)
{
  int result = 0;

  PDEBUG(srv, 1, "(%p)", srv);

  do {
    /* Create socket address. */
    srv->ep.socket_family = AF_UNIX;

    /* Delete old file. */
    {
      struct stat s;

      if ( stat(srv->opts.uds, &s) == 0 ) {
	if ( unlink(srv->opts.uds) < 0 ) {
	  perror(luxeed_error_action = "unlink");
	  // result = -1;
	  // break;
	}
      }
    }

    result = luxeed_server_open_socket(srv);
  } while ( 0 );

  PDEBUG(srv, 1, "(%p) => %d", srv, result);

  return result;
}


int luxeed_server_open_fifo(luxeed_server *srv)
{
  int result = 0;

  PDEBUG(srv, 1, "(%p)", srv);

  do {
    /* Create a named pipe (fifo). */
    if ( mknod(srv->server_path, S_IFIFO | 0666, 0) < 0 ) {
      perror(luxeed_error_action = "mknod");
      // result = -1;
      // break;
    }

    do {
      luxeed_client *cli;
      int client_fd;

      PDEBUG(srv, 1, ": open(%s) ...", srv->server_path);
      
      if ( (client_fd = open(srv->server_path, O_RDONLY)) < 0 ) {
	perror(luxeed_error_action = "open");
	result = -1;
	break;
      }

      PDEBUG(srv, 1, ": open(%s) => %d", srv->server_path, client_fd);

      /* Open client. */
      cli = malloc(sizeof(*cli));
      memset(cli, 0, sizeof(*cli));

      luxeed_client_open(cli, srv, client_fd, -1);

      /* Process events. */
      event_dispatch();
    } while ( 1 );
  } while ( 0 );

  PDEBUG(srv, 1, "(%p) => %d", srv, result);

  return result;
}


luxeed_device *luxeed_server_device(luxeed_server *srv)
{
  int result = 0;
  
  PDEBUG(srv, 1, "(%p)", srv);

  do {
    /* Allocate a device object. */
    if ( ! srv->dev ) {
      srv->dev = luxeed_device_create();
      srv->dev->opts = srv->opts;
    }

    /* Find the luxeed device. */
    if ( luxeed_device_find(srv->dev, 0, 0) ) {
      fprintf(stderr, "%s: luxeed keyboard not found\n", srv->opts.progname);
      luxeed_error_action = "luxeed_device_find";
      result = -1;
      break;
    }

    /* Attempt to open the device now. */
    if ( srv->dev ) {
      if ( (result = luxeed_device_open(srv->dev)) ) {
	luxeed_error_action = "luxeed_device_open";
	result = -1;
	break;
      }
    }

  } while ( 0 );

  return result ? 0 : srv->dev;
}


int luxeed_server_open(luxeed_server *srv)
{
  int result = 0;

  PDEBUG(srv, 1, "(%p)", srv);

  do {
    srv->ep.opts = srv->opts;

    // strcpy(srv->server_uri, "tcp://127.0.0.1:25324");
    strncpy(srv->server_path, "/tmp/luxeed", sizeof(srv->server_path));

    /* Open device here. */
    luxeed_server_device(srv);

    if ( srv->opts.fifo ) {
      if ( luxeed_server_open_fifo(srv) ) {
	luxeed_error_action = "luxeed_server_open_fifo";
	result = -1;
	break;
      }
    }

    if ( srv->opts.uds ) {
      if ( luxeed_server_open_uds(srv) ) {
	luxeed_error_action = "luxeed_server_open_uds";
	result = -1;
	break;
      }
    }

    if ( srv->opts.host ) {
      if ( luxeed_server_open_inet(srv) ) {
	luxeed_error_action = "luxeed_server_open_inet";
	result = -1;
	break;
      }
    }

  } while ( 0 );

  if ( result ) {
    luxeed_server_close(srv);
  }

  PDEBUG(srv, 1, "(%p) => %d", srv, result);

  return result;
}


int luxeed_server_close(luxeed_server *srv)
{
  int result = 0;

  if ( ! srv ) return 0;

  PDEBUG(srv, 1, "(%p)", srv);

  do {
    if ( srv->ep.accept_ev_active ) {
      srv->ep.accept_ev_active = 0;
      event_del(&srv->ep.accept_ev);
    }

    if ( srv->dev ) {
      luxeed_device_destroy(srv->dev);
    }

    unlink(srv->server_path);

    /* Close the device. */
    luxeed_device_destroy(srv->dev);
    srv->dev = 0;
  } while ( 0 );

  PDEBUG(srv, 1, "(%p) => %d", srv, result);

  return result;
}


/********************************************************************/


int luxeed_server_main(int argc, char **argv)
{
  int result = 0;
  luxeed_server _srv, *srv = &_srv;
 
  do {
    /* Initialize libevent. */
    event_init();
    
    /* Initialize server. */
    memset(srv, 0, sizeof(*srv));

    srv->opts = luxeed_options;

    if ( ! srv->opts.host ) {
      srv->opts.host = "127.0.0.1";
    }
    if ( ! srv->opts.port ) {
      srv->opts.port = 12345;
    }

    /* Open server and device. */
    result = luxeed_server_open(srv);
    if ( result ) break;

    /* Process events. */
    event_dispatch();

  } while ( 0 );

  if ( result ) {
    fprintf(stderr, "%s: error: %d\n", luxeed_options.progname, result);
    perror(luxeed_error_action ? luxeed_error_action : luxeed_options.progname);
  }

  /* Close server and device. */
  luxeed_server_close(srv);

  return result != 0 ? 1 : 0;
}


/* EOF */

