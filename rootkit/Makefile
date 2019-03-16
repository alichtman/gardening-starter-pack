KDIR = /lib/modules/$(shell uname -r)/build

all:
	mkdir -p dist
	make -C $(KDIR) M=$$PWD
clean:
	rm -rf dist
	rm -f config.h
	make -C $(KDIR) M=$$PWD clean
