CFLAGS    = -static -I./deps/include/
LDFLAGS   = -L./deps/lib/

BUILD     = build
OBJDIR    = $(BUILD)/obj
BINDIR    = $(BUILD)/bin

SRCS     = $(wildcard src/*.c)
BINS     = $(patsubst src/%.c, $(BINDIR)/%, $(SRCS))

INITRAMFS = initramfs

# 3P deps
cash_LIBS    = -leditline

.PHONY: all clean

all: $(BINS) initrd run

$(BINDIR)/% : $(OBJDIR)/%.o | $(BINDIR)
	gcc $(CFLAGS) $< $(LDFLAGS) $($*_LIBS) -o $@

$(OBJDIR)/%.o : src/%.c | $(OBJDIR)
	gcc $(CFLAGS) -c $< -o $@

$(OBJDIR) $(BINDIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD)
	rm -rf $(INITRAMFS)/

# Pretty sure this is not how makefiles are intended to be used
initrd:
	mkdir -p $(INITRAMFS)/{proc,sys,dev,bin,sbin,tmp}
	cp $(BINS) $(INITRAMFS)/bin/
	cd $(INITRAMFS) && \
	find . -print0 | cpio --null --create --verbose --format=newc | gzip --best > ../initrd.img

run:
	./run.sh
