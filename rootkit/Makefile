KDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$$PWD
clean:
	rm -f config.h
	make -C $(KDIR) M=$$PWD clean
