LIBS=-lusb -levent

all : products

install-prereqs :
	sudo apt-get install libusb-dev libevent-dev

products : luxeedd luxeed_test

luxeed_test: luxeed_test.c luxeed.c luxeed.h
	$(CC) -g -o $@ luxeed_test.c luxeed.c $(LIBS)

luxeedd: luxeedd.c luxeed.c luxeed.h
	$(CC) -g -o $@ luxeedd.c luxeed.c $(LIBS)

server: server.c
	$(CC) -g -o $@ server.c $(LIBS)

test: luxeed_test
	sudo ./luxeed_test 

debug: luxeed_test
	sudo gdb --args ./luxeed_test --eval-command='run'

debug-luxeedd: luxeedd
	sudo gdb --args $< --eval-command='run'

clean: 
	rm -f luxeed_test *.o

