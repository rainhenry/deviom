# deviom
Debugging tool for operating physical IO and memory space in user mode



Usually, in Ubuntu system, if we need to debug some low-level hardware resources, such as some registers, we usually use devmem2 or ioport (inb, outb, etc.).

This means our debugging efforts will rely on multiple tools. The ioport tool will be used to manipulate the x86-specific IO space. However, the ioport tool cannot manipulate IO space addresses in hexadecimal format because, as can be seen from its source code, it only interprets decimal (at least that's the case with the version I've seen so far).

At the same time, even if devmem2 is used to operate the more common physical memory space, there will be some usage problems. For example, it cannot operate address space above 64 bits, and actual measurements will produce errors.

Based on the above considerations, I decided to make a tool that is convenient for debugging. That is, the same tool can operate IO space and access memory space above 64 bits, and can operate in the familiar hexadecimal form. This gave rise to the tool - deviom



Using my tool is very simple. In Ubuntu system, you only need to install common compilation tools such as gcc and make to use it. The commands to install these dependencies are as follows:

```shell
sudo apt-get install gcc make build-essential
```

Then compile it:

```shell
make
```

Then there are examples of use, such as here to see its usage help:

```shell
sudo ./deviom -h
```

The output log is as follows:

```
[sudo] password for rainhenry: 
Usage: ./deviom [options]

Options:
  -h, --help               Show this help message
  -p, --port PORT          Specify I/O port number (hex)
  -m, --memory ADDR        Specify memory address (hex)
  -r, --read               Perform read operation
  -w, --write VALUE        Perform write operation with VALUE
  -W, --width WIDTH        Data width: 1(byte), 2(halfword), 4(word), 8(double word)
  -t, --type TYPE          Operation type: io(port), mem(memory)

Examples:
  ./deviom -t io -p 0x3f8 -r -W 1           # Read 1 byte from I/O port 0x3f8
  ./deviom -t io -p 0x3f8 -w 0x41 -W 1      # Write 0x41 to I/O port 0x3f8
  ./deviom -t mem -m 0x100000 -r -W 4       # Read 1 word from memory address 0x100000
  ./deviom -t mem -m 0x100000 -w 0x12345678 -W 4  # Write data to memory address 0x100000
```



Please note! When you try to access non-reserved space, such as mapped PMIO or MMIO space, please make sure your kernel does not have CONFIG_STRICT_DEVMEM enabled, otherwise the access will fail! !

Normally, kernels in distributions like Ubuntu will have CONFIG_STRICT_DEVMEM enabled by default for security reasons. Therefore, you need to re-download the kernel source code, recompile the kernel with CONFIG_STRICT_DEVMEM disabled, and install it. Only then can you access all physical IO and memory in user space normally.



Contact:

E-mail: rainhenry@savelife-tech.com

WeChat: (+86)-13104051251

