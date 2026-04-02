CFLAGS    = -static -I./deps/include/
LDFLAGS   = -L./deps/lib/
LDLIBS    = -leditline
INIT      = initproc
EXEC      = pwd
OBJS      = pwd.o
INITOBJ   = init.o
INITRAMFS = initramfs

.PHONY: all clean

all: $(INIT) $(EXEC) initrd run

$(INIT): $(INITOBJ)
	gcc $(CFLAGS) init.c $(LDFLAGS) $(LDLIBS) -o $(INIT)

$(EXEC): $(OBJS)
	gcc $(CFLAGS) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $(EXEC)

# Pretty sure this is not how makefiles are intended to be used
initrd:
	mkdir -p $(INITRAMFS)/{proc,sys,dev,bin,sbin,tmp}
	cd $(INITRAMFS) && \
	cp ../$(INIT) ./bin/init && \
	cp ../$(EXEC) ./bin/ && \
	find . -print0 | cpio --null --create --verbose --format=newc | gzip --best > ../initrd.img

run:
	qemu-system-x86_64 \
		-kernel arch/x86_64/boot/bzImage \
		-initrd initrd.img

clean:
	rm -rf $(OBJS) $(EXEC)
	rm -rf $(INITRAMFS)/
