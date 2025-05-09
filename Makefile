CC = gcc
CFLAGS = -static

TARGETS = memdump split_memdump
memdump_SRCS = dump_proc_memory.c read_memory.c
split_memdump_SRCS = split_memory.c read_memory.c

all: $(TARGETS)

memdump: $(memdump_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

split_memdump: $(split_memdump_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS)
