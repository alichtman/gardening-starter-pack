# linux-rootkit

### Usage

```bash
$ python3 setup.py install
```

### Objectives

1. Gain as much control of the system as possible.
2. Persist across restarts and make it hard to remove.
3. Hide as best as possible.

### Features

1. Delete log entries on the system so there are no logs of the attacker activities.
2. Replace `ps`, `top`, `netstat` and `lsof` to not show the processes which the rootkit is running.
3. Replace `login` to add a backdoor.
4. Hide/unhide arbitrary process.
5. Hide/unhide files/directories.

### Setting Up the Development Environment

Download an `Ubuntu 18.04.2 Bionic Beaver` VirtualBox or VMWare image from [osboxes](https://www.osboxes.org/ubuntu/). This should come with the `4.18.0-15-generic` kernel.

```bash
$ sudo apt-get update
$ sudo apt-get install gcc make libelf-dev git
$ git clone --recurse-submodules git@github.com:alichtman/gardening-starter-pack.git
```

### Technical Rundown

**TODO**

### Sources

1. https://aearnus.github.io/2018/06/21/writing-a-rootkit.html
2. https://web.archive.org/web/20160725125039/https://www.big-daddy.fr/repository/Documentation/Hacking/Security/Malware/Rootkits/writing-rootkit.txt
3. https://github.com/m0nad/Diamorphine
4. https://github.com/Aearnus/syscall-rootkit
5. [Intro to Kernel Modules](http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/)
6. [Character Devices](http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device/)
7. https://github.com/typoon/lkms/tree/master/genesis
8. https://gist.github.com/jvns/6894934
9. https://jvns.ca/blog/2013/10/08/day-6-i-wrote-a-rootkit/
10. https://0x00sec.org/t/kernel-rootkits-getting-your-hands-dirty/1485
11. https://github.com/mfontanini/Programs-Scripts/blob/master/rootkit/rootkit.c
12. https://github.com/hanj4096/wukong/blob/master/lkm/rootkit.c
13. https://github.com/a7vinx/liinux
14. https://git.teknik.io/Monstro/Rootorium/src/commit/adb48bb82dedf9f5164cf16515e77f4466d5dce9/rkkern/src/main.c<Paste>
15. https://github.com/NoviceLive/research-rootkit
17. https://github.com/bones-codes/the_colonel/blob/master/lkm/col_kl.c
18. [Magic Packets](https://www.drkns.net/kernel-who-does-magic/)
19. https://github.com/hanj4096/wukong/blob/master/lkm/rootkit.c
20. [How Reptile Works](https://github.com/milabs/awesome-linux-rootkits/blob/master/details/reptile.md)
21. [Reptile](https://github.com/f0rb1dd3n/Reptile)
22. [Awesome Linux Rootkits](https://github.com/milabs/awesome-linux-rootkits)
