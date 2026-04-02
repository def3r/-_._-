# Linux Kernel

## Kernel Config
- Use the `zdisable` script to disable unused drivers from
- Rebuild the linux kernel because using `zdisable` means that we changed
  kernel config.
- Kernel is compiled into a `Kernel: arch/x86/boot/bzImage is ready  (#3)` bzImage.
- This `bzImage` is the bootable kernel (without init proc btw)
- Config options not only contain drivers, but also virtualization features, file systems, etc.
- To see config opts tui: `make menuconfig`
- Very minimal "ready-made" config: `make tinyconfig` (holy thats small and fast to build)
- To disable all config opts: `make allnoconfig`

## Init Proc
- Init proc lives in the `initrd` (init ramdisk).
  - a tiny ramdisk the kernel mounts to before anything else

> [!NOTE]
> Q: Does init has to be `-freestanding`?
> A: No (needs `-static`, afaik yes)

## Build
- Using `make -j$(nproc)` creates the bootable kernal image `bzImage`.
- Boot it using something like:
```sh
qemu-system-x86_64 \
    -kernel arch/x86/boot/bzImage \
    -initrd initrd.img \
    -m 128M \
    -nographic \
    -append "console=ttyS0 quiet init=/initproc"
```
- To clean up stale `.o` files: `make mrproper`

## Minimal Filesystem
- Without using fs drivers and disk partitions, we can use filesystems: "initial ram fs" (`initramfs`)
- `initramfs`: you give linux a specific compressed archive contining some files and it will load it into RAM and use it as init root fs.

## Development

### Shell
- `readline` is good but not worth it. Reason? too many deps, `ncurses`, `termcap`/`tinfo`. For a static shell, can't aford that.
  - Replacement: `editline`, omg thats smooth, i love it!
- The `init` proc, `pwd` is `/`, and the envrionment vars need to be set by the `init`... well HOME is already set tho?
  - vars set for `init`: `HOME=/`, `TERM=linux`

# Resources:
1. [Is shared standard C library first initialized by kernel?](https://stackoverflow.com/questions/31623137/is-shared-standard-c-library-first-initialized-by-kernel)
2. [Is the standard C library loaded by default in main memory in Linux?](https://unix.stackexchange.com/questions/590108/is-the-standard-c-library-loaded-by-default-in-main-memory-in-linux)
3. [What is \__libc_start_main and \_start?](https://stackoverflow.com/questions/62709030/what-is-libc-start-main-and-start)
4. [What Does 'Not Syncing' Mean in Kernel Panic? Understanding the Error Message](https://linuxvox.com/blog/what-does-not-syncing-mean-in-kernel-panic/#understanding-syncing-in-the-kernel)
5. [Build and run a minimal Linux kernel](https://www.subrat.info/build-kernel-and-userspace/)
6. [Building a tiny linux](https://weeraman.com/building-a-tiny-linux-kernel/)
