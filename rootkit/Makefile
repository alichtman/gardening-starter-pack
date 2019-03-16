all:
	mkdir -p dist
	make -C /lib/modules/$(shell uname -r)/build M=$$PWD
clean:
	rm -rf dist
	rm -f config.h
	make -C /lib/modules/$(shell uname -r)/build M=$$PWD clean
