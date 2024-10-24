#include "luxeed_server.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h> /* inet_aton() */
#include <fcntl.h>


int luxeed_endpoint_init(luxeed_endpoint *ep, int init)
{
  int result = 0;

  PDEBUG(ep, 2, "(%p)", ep);

  /* Clear file descriptors. */
  ep->in_fd = ep->out_fd = -1;
  ep->in = ep->out = 0;

  /* Select the socket type. */
  switch ( ep->socket_family ) {
  case AF_INET:
    memset(&ep->inet_addr, 0, sizeof(ep->inet_addr));
    ep->inet_addr.sin_family = AF_INET;
    ep->socket_addr = (void*) &ep->inet_addr;
    ep->socket_addr_size = sizeof(ep->inet_addr);
    if ( init ) {
      /* Bind to address */
      PDEBUG(ep, 3, "(%p) AF_INET %s:%d", ep, ep->opts.host, ep->opts.port);
      ep->inet_addr.sin_port = htons(ep->opts.port);
      if ( inet_aton(ep->opts.host, &ep->inet_addr.sin_addr) == 0 ) {
        perror(luxeed_error_action = "inet_aton");
        result = -1;
        goto done;
      }
    }
    break;

  case AF_UNIX:
    memset(&ep->uds_addr, 0, sizeof(ep->uds_addr));
    ep->uds_addr.sun_family = AF_UNIX;
    ep->socket_addr = (void*) &ep->uds_addr;
    ep->socket_addr_size = sizeof(ep->uds_addr);
    break;

  default:
    result = -1;
    break;
  }

  done:
  PDEBUG(ep, 2, "(%p) => %d", ep, result);

  return result;
}


int luxeed_endpoint_bind(luxeed_endpoint *srv)
{
  int result = 0;

  PDEBUG(srv, 2, "(%p)", srv);

  do {
    /* Prepare socket address. */
    if ( luxeed_endpoint_init(srv, 1) ) {
      perror(luxeed_error_action = "luxeed_endpoint_init()");
      result = -1;
      break;
    }

    /* Open socket. */
    PDEBUG(srv, 2, "(%p) : Open socket.", srv);
    if ( (srv->in_fd = socket(srv->socket_family, SOCK_STREAM, 0)) < 0 ) {
      perror(luxeed_error_action = "socket");
      result = -1;
      break;
    }

    /* Reuse address. */
#ifdef SO_REUSEADDR
    {
      int option = 1;

      PDEBUG(srv, 2, "(%p) : setsockopt(%d, SO_REUSEADDR)", srv, srv->in_fd);
      if ( setsockopt(srv->in_fd, SOCK_STREAM, SO_REUSEADDR, &option, sizeof(option)) < 0 ) {
      	perror(luxeed_error_action = "setsockopt: SO_REUSEADDR");
      }
    }
#endif

    /* Do not linger. */
    PDEBUG(srv, 2, "(%p) : setsockopt(%d, SO_LINGER)", srv, srv->in_fd);
    {
      struct linger l = { 0, 0 };
      if ( setsockopt(srv->in_fd, SOCK_STREAM, SO_LINGER, &l, sizeof(l)) < 0 ) {
        perror(luxeed_error_action = "setsockopt: SO_LINGER");
      }
    }

    /* Bind address. */
    PDEBUG(srv, 2, "(%p) : bind(%d, ...)", srv, srv->in_fd);
    if ( bind(srv->in_fd,
	      (const struct sockaddr *) srv->socket_addr,
        (socklen_t) srv->socket_addr_size
        ) < 0 ) {
      perror(luxeed_error_action = "bind");
      result = -1;
      break;
    }

    /* Set perms/ownership. */
    if ( srv->opts.fifo || srv->opts.uds ) {
      if ( fchmod(srv->in_fd, 0644) < 0 ) {
        perror(luxeed_error_action = "fchmod(srv->fd, ...)");
        // result = -1;
        // break;
      }
    }

    /* Listen. */
    PDEBUG(srv, 2, "(%p) : bind(%d, ...)", srv, srv->in_fd);
    if ( listen(srv->in_fd, 5) < 0 ) {
      perror(luxeed_error_action = "listen");
      result = -1;
      break;
    }
  } while ( 0 );

  PDEBUG(srv, 2, "(%p) => %d", srv, result);

  return result;
}


int luxeed_endpoint_accept(luxeed_endpoint *srv, luxeed_endpoint *cli)
{
  int result = 0;

  PDEBUG(srv, 2, "(%p)", srv);

  do {
    cli->socket_family = srv->socket_family;
    if ( luxeed_endpoint_init(cli, 0) ) {
      perror(luxeed_error_action = "luxeed_endpoint_init()");
      result = -1;
      break;
    }

    if ( (cli->in_fd = accept(srv->in_fd,
			      (struct sockaddr *) cli->socket_addr,
			      &cli->socket_addr_size)
	  ) < 0 ) {
      perror(luxeed_error_action = "accept()");
      result = -1;
      break;
    }

    /* Do not linger. */
    {
      struct linger l = { 0, 0 };
      if ( setsockopt(cli->in_fd, SOCK_STREAM, SO_LINGER, &l, sizeof(l)) < 0 ) {
	perror(luxeed_error_action = "setsockopt: SO_LINGER");
      }
    }
  } while ( 0 );

  PDEBUG(srv, 2, "(%p) => %d", srv, result);

  return result;
}


int luxeed_endpoint_open(luxeed_endpoint *ep, int in_fd, int out_fd)
{
  int result = 0;

  if ( ! ep ) return 0;

  PDEBUG(ep, 2, "(%p, %d, %d)", ep, (int) in_fd, (int) out_fd);

  ep->buf_size = sizeof(ep->buf);
  ep->buf_len = 0;

  ep->in_fd = in_fd;
  ep->out_fd = out_fd;
  ep->in = fdopen(ep->in_fd, "r");
  if ( ep->out_fd >= 0 ) {
    ep->out = fdopen(ep->out_fd, "w");
  }

  /* Force buffering only up till newline so that select() still
  ** has other read(0) pending.
  */
  setlinebuf(ep->in);

  PDEBUG(ep, 2, "(%p) => %d", ep, result);

  return result;
}


int luxeed_endpoint_close(luxeed_endpoint *ep)
{
  int result = 0;

  if ( ! ep ) return 0;

  PDEBUG(ep, 2, "(%p): %d, %d", ep, (int) ep->in_fd, (int) ep->out_fd);

  if ( ep->in )
    fclose(ep->in);
  ep->in = 0;

  if ( ep->out )
    fclose(ep->out);
  ep->out = 0;

  if ( ep->in_fd >= 0 )
    close(ep->in_fd);
  ep->in_fd = -1;

  if ( ep->out_fd >= 0 )
    close(ep->out_fd);
  ep->out_fd = -1;

  PDEBUG(ep, 2, "(%p) => %d", ep, result);

  return result;
}


int luxeed_endpoint_read_line(luxeed_endpoint *ep, char *buf, size_t buf_size)
{
  int result = 0;
  void *x;

  PDEBUG(ep, 3, "(%p)", ep);

  x = fgets(buf, buf_size, ep->in);

  if ( x == 0 ) {
    result = -1;
  }

  PDEBUG(ep, 3, "(%p) => %d", ep, result);

  return result;
}


/* EOF */

