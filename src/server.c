
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "event.h"


static struct event luxeedd_accept_client_ev;
static
int luxeedd_accept_client(int fd, short event, void *data)
{
  event_set(&luxeedd_read_command_ev, 0, EV_READ, &luxeedd_read_command, 0); 
  event_add(&luxeedd_read_command_ev, 0);

  return 0;
}


static struct event luxeedd_read_command_ev;
static
int luxeedd_read_command(int fd, short event, void *data)
{
  char buf[1024];
  size_t bufsize = sizeof(buf);
  int result;

  result = read(fd, buf, bufsize);
  if ( result > 0 ) {
    event_add(&luxeedd_read_command_ev, 0);

    write(1, "> ", 2);
    write(1, buf, result);
    write(1, "\n", 1);
  }

  return 0;
}


int main(int argc, char **argv)
{
  event_init();

  event_set(&luxeedd_read_command_ev, 0, EV_READ, &luxeedd_read_command, 0); 
  event_add(&luxeedd_read_command_ev, 0);

  event_dispatch();
  
  return 0;
}
