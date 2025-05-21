#define _LARGEFILE64_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef READ_MEM
#define READ_MEM

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#ifndef PAGEMAP_ENTRY
#define PAGEMAP_ENTRY 8
#endif

typedef enum { HEAP, ANON, LIBRARY, STACK, OTHER, FILEMAPPED } entry_type;

typedef struct {
  unsigned long long start_addr; // virtual start address
  unsigned long long end_addr;   // virtual end address
  char pathname[100]; // path, last field for the entry in the maps file
  int valid;
  entry_type entry_type;
  int present_pages;   // number of pages present in memory
  char shared_private; // is the entry private or shared (inode field)
  char execute;        // is the entry executable or not
} Maps_entry;

void categorize_entry(Maps_entry *map);
int wr_maps_entry_to_file(int pid, Maps_entry *map, FILE *bin_out,
                          FILE *type_out);
void get_present_pages(int pid, Maps_entry *map);
void valid_entry(Maps_entry *map);
void print_maps_entry(Maps_entry *maps_entry);
int read_maps_file(int pid, FILE *file_bin_out, FILE *file_maps_out,
                   FILE *file_type_out);

#endif
