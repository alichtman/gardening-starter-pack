# Rootkit Linux Kernel Module Makefile
# This will produce a rootkit.ko file in the current directory.

obj-m+=rootkit.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
