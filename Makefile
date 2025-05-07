split_memdump: split_memory.c
	gcc -Wall -static -o split_memdump split_memory.c read_memory.c

dump_proc_memory: dump_proc_memory.c
	gcc -Wall -static -o memdump dump_proc_memory.c read_memory.c

all: dump_proc_memory.c
	gcc -Wall -static -o memdump dump_proc_memory.c read_memory.c

clean:
	rm memdump
