LIBS=-lusb

all : products

install_prereqs :
	sudo apt-get install libusb-dev

products : luxeed_test

luxeed_test: luxeed_test.c luxeed.c luxeed.h
	$(CC) -g -o $@ luxeed_test.c luxeed.c $(LIBS)

test: luxeed_test
	sudo ./luxeed_test 

debug: luxeed_test
	sudo gdb --args ./luxeed_test --eval-command='run'

clean: 
	rm -f luxeed_test *.o

