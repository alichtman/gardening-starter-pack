### Architecture

This rootkit mainly relies on the [`khook`](https://github.com/milabs/khook) library, which uses ROP chains to hook syscalls without modifying the pointers in the syscall table or touching the IDT / IDTR.

There is a set of diagrams in the `khook` README that lays out exactly how it works. I've included a modified version below to illustrate this concept.

---

This diagram shows a call to syscall `X` without hooking:

```
CALLER
| ...
| CALL X -(1)---> X
| ...  <----.     | ...
` RET       |     ` RET -.
            `--------(2)-'
```

This diagram shows a call to syscall `X` when `khook` is used:

```
CALLER
| ...
| CALL X -(1)---> X
| ...  <----.     | JUMP -(2)----> STUB.hook
` RET       |     | ???            | INCR use_count
            |     | ...  <----.    | CALL handler -(3)------> HOOK.fn
            |     | ...       |    | DECR use_count <----.    | ...
            |     ` RET -.    |    ` RET -.              |    | CALL origin -(4)------> STUB.orig
            |            |    |           |              |    | ...  <----.             | N bytes of X
            |            |    |           |              |    ` RET -.    |             ` JMP X + N -.
            `------------|----|-------(8)-'              '-------(7)-'    |                          |
                         |    `-------------------------------------------|----------------------(5)-'
                         `-(6)--------------------------------------------'
```

---

This method of hooking syscalls is currently undetected by `chkrootkit`, as it does not modify the Interrupt Descriptor Table (IDT), Interrupt Descriptor Table Register (IDTR) or the Syscall Table. It can, however, be detected by [`Tyton`](https://github.com/nbulischeck/tyton). I have reproduced both of these claims.

### Rootkit Configuration

The rootkit configuration process is done interactively using the `setup.py` script.

There are four functionalities you can currently use:

1. Hide files/folders.
2. Block removal of the rootkit.
3. Escalate privileges to root.
4. Send pings to machine regardless of firewall rules (since the `NetFilter` stack lives in the kernel and `iptables` lives above the kernel).

### How do you interact with the rootkit?

A userspace program to control the rootkit, called `rootkit_command.c`, is installed at `/garden`. (In a future update, it will be installed using whatever driver name you supply during the setup phase). In order to interact with the rootkit, simply run the command `$ /garden`.

### So how does it actually work? What syscalls are hooked?

**TODO**: All the documentation for this section is currently written in `C`. Check out the `rootkit/*.c` files for the good stuff.
