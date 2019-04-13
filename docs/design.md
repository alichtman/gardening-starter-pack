### Architecture

This rootkit takes advantage of the [`khook`](https://github.com/milabs/khook) library, which uses ROP chains to hook syscalls.

There is a set of diagrams in the `khook` README that lays out exactly how it works. I've included a modified version below for completeness.

---

This diagram illustrates a call to function `X` without hooking:

```
CALLER
| ...
| CALL X -(1)---> X
| ...  <----.     | ...
` RET       |     ` RET -.
            `--------(2)-'
```

This diagram illustrates a call to function `X` when `khook` is used:

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
~~~

---

This method of hooking syscalls is currently undetected by `chkrootkit`.

### Rootkit configuration

The rootkit configuration process will be done interactively using the `setup.py` script.

There are three functionalities you can currently use:

1. Hide files/folders.
2. Block removal of the rootkit.
3. Escalate privileges to root.

<!-- 3. reverse_shell_ip -->

### How do you interact with the rootkit?

Kernel module variables are stored in `/sys/module/<MODULE_NAME>/parameters/<PARAM_NAME>`. Since this rootkit is a kernel module, and all the configuration of the rootkit is done with parameters, these parameters are theoretically writable at runtime (after messing with the file permissions in the setup script.)

If you wanted to change the `reverse_shell_ip` on the fly, and the module name is `garden`, the command would look like this: `$ echo "$IP > /sys/module/garden/parameters/reverse_shell_ip`.

I've created symlinks at `/<MODULE_NAME>/<PARAM>` to shorten the commands up, so all you need to enter is: `$ echo "$IP > /garden/reverse_shell_ip`

**NOTE: You can not enable removal of the rootkit once you've enabled the `block_removal` toggle.**

### So how does it actually work? What syscalls are hooked?

**TODO**
