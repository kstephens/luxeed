LIBS=-L. -lluxeed -lusb -levent #
#LIBS+= -largp #
LIBLUXEED_C = \
  luxeed_misc.c \
  luxeed_device.c \
  luxeed_endpoint.c \
  luxeed_server.c

LIBLUXEED_O=$(LIBLUXEED_C:.c=.o)

CFLAGS+=-g -O2 -Wall

all : products

install-prereqs :
	sudo apt-get install libusb-dev libevent-dev # libargp-dev

PRODUCTS = libluxeed.a luxeed luxeedd luxeed_test

products : $(PRODUCTS)

libluxeed.a : $(LIBLUXEED_O) Makefile
	ar r $@ $(LIBLUXEED_O)
	ranlib $@

$(LIBLUXEED_O) : *.h Makefile

luxeed_test: luxeed_test.c libluxeed.a *.h 
	$(CC) $(CFLAGS) -o $@ luxeed_test.c $(LIBS)

luxeedd: luxeedd.c libluxeed.a *.h 
	$(CC) $(CFLAGS) -o $@ luxeedd.c $(LIBS)

luxeed: luxeed.c libluxeed.a  *.h
	$(CC) $(CFLAGS) -o $@ luxeed.c $(LIBS)

$(PRODUCTS) : Makefile

test: luxeed_test
	sudo ./luxeed_test 

debug-test: luxeed_test
	sudo gdb --eval-command='run' --args ./luxeed_test 

debug: luxeed
	sudo gdb --eval-command='run' --args ./luxeed --server

debug-luxeedd: luxeedd
	sudo gdb --eval-command='run' --args ./luxeedd

clean: 
	rm -f $(PRODUCTS) *.o lib*.a

