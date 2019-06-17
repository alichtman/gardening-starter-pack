**Kernel Modules**


1. Handles requests which it registers to handle using its initialization function, which runs and then terminates. Quite similar to event-driven programming model.
2. No automatic cleanup. Any resources allocated must be manually freed.
3. Kernel code cannot access user space libraries of code since it runs in kernel space, which has its own memory address space. This means no `printf()`. Use `printk()` to print information instead.
4. Can be interrupted. Can be used by several different programs/processes at the same time. Need to carefully construct modules so that they have a consistent and valid behavior when they are interrupted. Consider the impact of multiple processes accessing the module simultaneously.
5. Have a high level of execution privilege.

Source: http://derekmolloy.ie/writing-a-linux-kernel-module-part-1-introduction/
