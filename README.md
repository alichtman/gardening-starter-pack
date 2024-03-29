# Gardening-Starter-Pack
> Quite Literally a Rootkit

<!-- <h1 align="center">
  <img src="img/Garden.png" width="75%" />
  <br />
</h1> -->

### Disclaimer

This codebase was developed for purely educational reasons. It is illegal to run this code on a machine that is not your own, or you do not have permission to run this on.

### Usage

To install, simply run `$ sudo python3 setup.py install`.
To remove, run `$ sudo python3 setup.py uninstall`

If you'd be more comfortable reading these same options in your terminal, run `$ python3 setup.py -h`.

### Configuration Notes

- The reverse shell is broken, and commented out at the moment. It causes a kernel panic whenver it is invoked. Still working on this problem.
- No guarantees on things working if you use non-default configuration names. I have not tested every route through this code.

A known good configuration sequence is:

```bash
$ sudo python3 setup.py install
<ENTER>
Y
N
Y
<ENTER>
Y
```

You'll know things have worked properly if you run `$ kill 31337` and are dropped into a root shell. The `/garden` binary should not be visible when you run `$ ls /`, even though the command `/garden` will work. The output for `lsmod` should not include `garden` if you've followed the config above.

### Features

1. Hide/unhide files/directories.
2. Escalate priveleges to root.
3. Listen for magic packets (will not be stopped by local firewall) to spawn reverse shell.
4. Hide rootkit.
5. Block uninstallation of rootkit.
6. Reboot persistence.

NOTE: Reverse shell is currently broken. I need to read / understand more about kernel-threads in order to implement that part of the rootkit. The magic packet listener is fully functional.

### Tested Kernels

- `4.15.0-15-generic`
- `4.18.0-16-generic`
- `4.18.0-17-generic`

Theoretically, this rootkit will be compatible with every kernel above `4.14`, but these are the only kernels that have been tested.

### Warning

If you choose to develop on real hardware, make sure you have a full system backup. If you install the rootkit with the "block uninstallation" option toggled, you **will not be able to uninstall it.** Your only choice for recovery is a full OS reinstall. (At least, that I am aware of / was able to figure out. I had to re-image my VM a few times...)

### Setting Up the Development Environment

Download an `Ubuntu 18.04.2 Bionic Beaver` VirtualBox or VMWare image from [osboxes](https://www.osboxes.org/ubuntu/). This should come with the `4.18.0-15-generic` kernel.

Make sure you have a version of `Python 3.X` installed. I wrote the build script in `Python 3.6.7`, but anything that's `3.0` or above should work.

```bash
$ sudo apt-get update
$ sudo apt-get install gcc make libelf-dev git
# This will not work without my private SSH key.
$ git clone --recurse-submodules git@github.com:alichtman/gardening-starter-pack.git
```

Then, (for long-term development) add this line to your crontab to deal with the absurd number of debug logs created: `0 * * * * sudo ~/gardening-starter-pack/scripts/clean_vm.sh`

### Technical Details

See the `docs/` directory.

### Acknowledgements

Here are some books, tutorials and projects that helped me as I was writing this.

**Linux Kernel Development**

1. [Intro to Kernel Modules](http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/)
2. [Character Devices](http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device/)
3. [Linux Kernel Development 3rd Edition](https://smtebooks.com/book/1852/linux-kernel-development-3rd-pdf-1)
4. [Linux Device Drivers 3rd Edition](https://www.oreilly.com/library/view/linux-device-drivers/0596005903/)

**Rootkit Development**

1. [Intro to Writing Kernel Rootkits](https://0x00sec.org/t/kernel-rootkits-getting-your-hands-dirty/1485)
2. [Reptile Rootkit](https://github.com/f0rb1dd3n/Reptile)
3. [How Reptile Works](https://github.com/milabs/awesome-linux-rootkits/blob/master/details/reptile.md)
4. [mfontanini's Rootkit](https://github.com/mfontanini/Programs-Scripts/blob/master/rootkit/rootkit.c)
5. [hanj0496's Rootkit](https://github.com/hanj4096/wukong/blob/master/lkm/rootkit.c)
6. [a7vinx's Rootkit](https://github.com/a7vinx/liinux)
7. [NoviceLive's Kernel Rootkit Tutorial/Analysis](https://github.com/NoviceLive/research-rootkit)
8. [Bones-codes' Rootkit](https://github.com/bones-codes/the_colonel/)
9. [Magic Packets](https://www.drkns.net/kernel-who-does-magic/)
