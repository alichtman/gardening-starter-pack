# linux-rootkit

### Usage

```bash
$ make # Compile the rootkit 
$ sudo insmod rootkit.ko # Load this module into the kernel.
$ lsmod # Display loaded kernel modules
$ sudo rmmod rootkit.ko # Unload rootkit.
$ sudo tail /var/log/kern.log # Print last 10 kernel log messages
```

It's possible that by default the device is only accessible by root so we need to change the permissions after loading our module with:

`$ sudo chmod 0666 /dev/<MODULE_NAME>`

We should try to modify the module so the device is created with the correct permissions.

### Rootkit Objectives

1. Gain as much control of the system as possible.
2. Persist across restarts and make it hard to remove.
3. Hide as best as possible.

### Features

1. Modify the kernel’s syscall table and replace a system call to point to a program of the rootkit.
2. Delete log entries on the system so there are no logs of the attacker activities.
3. Replace `ps`, `top`, `netstat` and `lsof` to not show the processes which the rootkit is running.
4. Replace `login` to add a backdoor.
5. Hide/unhide arbitrary process.
6. Hide/unhide files/directories.

### Potential Approaches

1. Look up kernel syscall table address using `kallsyms_lookup_name(<FUNC>)` from `linux/kallsyms.h`. Replace our custom syscalls with the actual ones, and then swap the originals back in after we've done what we want.


### Setting Up the Development Environment

```bash
$ sudo apt-get update
$ apt-cache search linux-headers-$(uname -r)
linux-headers-<VERSION>-<ARCH> - Header files for Linux <VERSION>-<ARCH>
$ sudo apt-get install linux-headers-<VERSION>-<ARCH>
$ cd /usr/src/linux-headers-<VERSION>-<ARCH>/
$ ls
arch  include  Makefile  Module.symvers  scripts
```

### Sources

1. https://aearnus.github.io/2018/06/21/writing-a-rootkit.html
2. https://web.archive.org/web/20160725125039/https://www.big-daddy.fr/repository/Documentation/Hacking/Security/Malware/Rootkits/writing-rootkit.txt
3. https://github.com/m0nad/Diamorphine
4. https://github.com/Aearnus/syscall-rootkit
5. http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/
6. http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device/
7. https://github.com/typoon/lkms/tree/master/genesis
8. https://gist.github.com/jvns/6894934
9. https://jvns.ca/blog/2013/10/08/day-6-i-wrote-a-rootkit/
10. https://0x00sec.org/t/kernel-rootkits-getting-your-hands-dirty/1485
11. https://github.com/mfontanini/Programs-Scripts/blob/master/rootkit/rootkit.c
