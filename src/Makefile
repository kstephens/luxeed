# on OS X: sudo port install libusb-compat argp-standalone libevent
LIBS=-L. -lluxeed -L/opt/local/lib -lusb -levent -largp #
#LIBS+= -largp #
LIBLUXEED_C = \
  luxeed_device.c \
  luxeed_endpoint.c \
  luxeed_server.c \
  luxeed_command.c \
  luxeed_misc.c

LIBLUXEED_O=$(LIBLUXEED_C:.c=.o)

CFLAGS+=-g -O2 -Wall -I/opt/local/include

all : products

install-prereqs :
	sudo apt-get install libusb-dev libevent-dev # libargp-dev

PRODUCTS = libluxeed.a luxeed luxeed_test

products : $(PRODUCTS)

libluxeed.a : $(LIBLUXEED_O) Makefile
	ar r $@ $(LIBLUXEED_O)
	ranlib $@

$(LIBLUXEED_O) : *.h Makefile

luxeed_test: luxeed_test.c libluxeed.a *.h
	$(CC) $(CFLAGS) -o $@ luxeed_test.c $(LIBS)

luxeed: luxeed.c libluxeed.a  *.h
	$(CC) $(CFLAGS) -o $@ luxeed.c $(LIBS)

$(PRODUCTS) : Makefile

test: luxeed_test
	sudo ./luxeed_test

debug-test: luxeed_test
	sudo gdb --eval-command='run' --args ./luxeed_test

debug: luxeed
	sudo gdb --eval-command='run' --args ./luxeed --server

clean:
	rm -f $(PRODUCTS) *.o lib*.a

