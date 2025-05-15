#include "read_memory.h"
#include <assert.h>
#include <string.h>

const int __endian_bit = 1;
#define GET_BIT(X, Y) ((X) & ((uint64_t)1 << (Y))) >> (Y)
#define GET_PFN(X) (X) & 0x7FFFFFFFFFFFFF

#define GET_BIT8(bit, val)
#define SET_BIT8(bit, val)

#define is_bigendian() ((*(char *)&__endian_bit) == 0)

void categorize_entry(Maps_entry *map) {
  int str_len;
  char subbuff[6];

  str_len = strlen(map->pathname);
  map->entry_type = OTHER;

  if (str_len >= 6) {
    memcpy(subbuff, map->pathname + 1, 5);
    subbuff[5] = '\0';
    if (!strcmp("stack", subbuff)) {
      map->entry_type = STACK;
      return;
    }
  }

  if (str_len >= 5) {
    memcpy(subbuff, map->pathname + 1, 4);
    subbuff[4] = '\0';
    if (!strcmp("heap", subbuff)) {
      map->entry_type = HEAP;
      return;
    }
  }

  if (str_len == 0) {
    map->entry_type = ANON;
  }

  if (str_len >= 1) {
    if (map->pathname[0] == '/') {
      map->entry_type = LIBRARY;
      return;
    }
  }
}

int wr_maps_entry_to_file(int pid, Maps_entry *map, FILE *bin_out,
                          FILE *type_out) {
  char mem_file_name[100], path_pagemap[0x100], *buf;
  int i, c, num_read_pages, mem_fd, status = 0;
  uint64_t file_offset, read_val;
  FILE *file_pagemap;
  unsigned long long addr;
  unsigned char c_buf[PAGEMAP_ENTRY];
  num_read_pages = 0;

  sprintf(path_pagemap, "/proc/%u/pagemap", pid);
  file_pagemap = fopen(path_pagemap, "rb");

  if (!file_pagemap) {
    printf("Error! Cannot open %s\n", path_pagemap);
    return ENOENT;
  }

  for (addr = map->start_addr; addr < map->end_addr; addr += PAGE_SIZE) {

    file_offset = addr / getpagesize() * PAGEMAP_ENTRY;
    status = fseek(file_pagemap, file_offset, SEEK_SET);

    if (status) {
      printf("Failed to do fseek!\n");
      return EIO;
    }

    errno = 0;
    read_val = 0;
    for (i = 0; i < PAGEMAP_ENTRY; i++) {
      c = getc(file_pagemap);
      if (c == EOF) {
        printf("\nReached end of the file\n");
      }
      if (is_bigendian()) {
        c_buf[i] = c;
      } else {
        c_buf[PAGEMAP_ENTRY - i - 1] = c;
      }
    }
    for (i = 0; i < PAGEMAP_ENTRY; i++) {
      read_val = (read_val << 8) + c_buf[i];
    }

    if (GET_BIT(read_val, 63)) {
      buf = (char *)malloc(PAGE_SIZE);

      /* print the pid and the pfn */
      printf("<%d::%llx>\n", pid, (long long unsigned int)GET_PFN(read_val));
      sprintf(mem_file_name, "/proc/%d/mem", pid);
      mem_fd = open(mem_file_name, O_RDONLY);

      if (mem_fd == -1) {
        printf("ERROR: cannot open /proc/%d/mem\n", pid);
      }
      if (ptrace(PTRACE_ATTACH, pid, NULL, NULL)) {
        printf("ERROR: attach PTRACE\n");
      }
      waitpid(pid, NULL, 0);
      if (lseek64(mem_fd, addr, SEEK_SET) == -1) {
        printf("ERROR: cannot lseek in /proc/%d/mem virtaddr: %llx\n", pid,
               addr);
      }

      // read the page
      if (read(mem_fd, buf, PAGE_SIZE) == -1) {
        printf("ERROR: cannot read from /proc/%d/mem\n", pid);
      }

      close(mem_fd);
      if (ptrace(PTRACE_DETACH, pid, NULL, NULL)) {
        printf("ERROR: detach PTRACE\n");
      }
      num_read_pages++;

      // write the page to the output file
      fwrite(buf, PAGE_SIZE, 1, bin_out);

      // update type
      if (GET_BIT(read_val, 61)) {
        map->entry_type = FILEMAPPED;
      }

      // write the classification of the page to the output file
      fwrite(&map->entry_type, sizeof(map->entry_type), 1, type_out);

      close(mem_fd);

      free(buf);
    }
  }
  fclose(file_pagemap);
  map->present_pages = num_read_pages;

  printf("name:%s  shared:%c pages:%d type:%d execute:%c\n", map->pathname,
         map->shared_private, map->present_pages, map->entry_type,
         map->execute);

  return status;
}

void get_present_pages(int pid, Maps_entry *map) {
  char path_buf[0x100];
  int i, c, status, num_read_pages;
  uint64_t read_val, file_offset;
  FILE *f;
  unsigned char c_buf[PAGEMAP_ENTRY];

  num_read_pages = 0;
  sprintf(path_buf, "/proc/%u/pagemap", pid);

  f = fopen(path_buf, "rb");
  if (!f) {
    printf("Error! Cannot open %s\n", path_buf);
  }
  unsigned long long iii;
  for (iii = map->start_addr; iii < map->end_addr; iii += PAGE_SIZE) {

    file_offset = iii / getpagesize() * PAGEMAP_ENTRY;
    status = fseeko(f, file_offset, SEEK_SET);
    if (status) {
      perror("Failed to do fseek!");
    }

    errno = 0;
    read_val = 0;
    for (i = 0; i < PAGEMAP_ENTRY; i++) {
      c = getc(f);

      if (c == EOF) {
        printf("\nReached end of the file\n");
      }
      if (is_bigendian())
        c_buf[i] = c;
      else
        c_buf[PAGEMAP_ENTRY - i - 1] = c;
    }
    for (i = 0; i < PAGEMAP_ENTRY; i++) {
      read_val = (read_val << 8) + c_buf[i];
    }

    if (GET_BIT(read_val, 63)) {
      num_read_pages++;
    }
  }
  fclose(f);
  map->present_pages = num_read_pages;
}

void print_maps_entry(Maps_entry *map) {
  printf("start: 0x%llx stop: 0x%llx pathname: \"%s\" valid: %d\n",
         map->start_addr, map->end_addr, map->pathname, map->valid);
}

void valid_entry(Maps_entry *map) {
  /* check if the maps entry has been deleted */

  int str_len;
  str_len = strlen(map->pathname);

  if (!strcmp("(deleted)", map->pathname + (str_len - 9))) {
    map->valid = 0;
  } else {
    map->valid = 1;
  }
}

int read_maps_file(int pid, FILE *file_bin_out, FILE *file_maps_out,
                   FILE *type_out) {

  char path_maps[100], start_addr[0x100], end_addr[0x100], *curr_addr,
      shared_private, execute;
  FILE *file_maps;
  int c, maps_entries, pos_addr, pos_pathname, field, entry, i, num_heap_pages,
      num_anon_pages, num_stack_pages, num_library_pages, num_other_pages,
      num_filemapped_pages;
  Maps_entry *maps_arr;

  sprintf(path_maps, "/proc/%u/maps", pid);
  file_maps = fopen(path_maps, "r");

  if (file_maps == NULL) {
    printf("could not open file: %s\n", path_maps);
    return ENOENT;
  }

  /* Count rows/entries in the maps file */
  maps_entries = 0;
  while ((c = fgetc(file_maps)) != EOF) {
    if (c == '\n') {
      maps_entries++;
    }
  }
  fclose(file_maps);

  maps_arr = malloc(maps_entries * sizeof(Maps_entry));
  file_maps = fopen(path_maps, "r");
  pos_addr = 0;
  pos_pathname = 0;
  field = 0;
  entry = 0;
  curr_addr = start_addr;

  while ((c = fgetc(file_maps)) != EOF) {
    /* parse the entries in the maps file */

    fwrite(&c, 1, 1, file_maps_out);
    if (field == 0) {
      if (c == '-') {
        curr_addr[pos_addr] = '\0';
        pos_addr = 0;
        curr_addr = end_addr;
      } else if (c == ' ') {
        field++;
        curr_addr[pos_addr] = '\0';
        curr_addr = start_addr;

        /* virtual addresses for start and end addresses */
        maps_arr[entry].start_addr = strtoll(start_addr, NULL, 16);
        maps_arr[entry].end_addr = strtoll(end_addr, NULL, 16);
        pos_addr = 0;
      } else {
        curr_addr[pos_addr] = (char)c;
        pos_addr++;
      }

    } else if (field == 1) {
      if (c == ' ') {
        /* last char before the ' ' either s/p */
        maps_arr[entry].shared_private = shared_private;
        maps_arr[entry].execute = execute;
        field++;
      }
      execute = shared_private;
      shared_private = c;

    } else if (field > 1 && field < 5) {
      /* do not track these maps fields */
      if (c == ' ') {
        field++;
      }
    } else if (field == 5) {
      if (c == '\n') {
        /* finied maps entry */
        maps_arr[entry].pathname[pos_pathname] = '\0';
        pos_pathname = 0;
        field = 0;
        entry++;

      } else if (c != ' ') {
        maps_arr[entry].pathname[pos_pathname] = c;
        pos_pathname++;
      }
    }
  }
  fclose(file_maps);

  /* analyze the entries in the maps file */
  for (i = 0; i < maps_entries; i++) {
    valid_entry(&maps_arr[i]);

    if (maps_arr[i].valid) {
      categorize_entry(&maps_arr[i]);
      wr_maps_entry_to_file(pid, &maps_arr[i], file_bin_out, type_out);

      if (maps_arr[i].entry_type == OTHER) {
        printf("OTHER: %d %s\n", maps_arr[i].present_pages,
               maps_arr[i].pathname);
      }
    }
  }

  num_heap_pages = 0;
  num_anon_pages = 0;
  num_stack_pages = 0;
  num_library_pages = 0;
  num_other_pages = 0;
  num_filemapped_pages = 0;

  printf(
      "pid: %d heap: %d anon: %d stack: %d lib: %d other: %d filemapped: %d\n",
      pid, num_heap_pages, num_anon_pages, num_stack_pages, num_library_pages,
      num_other_pages, num_filemapped_pages);

  for (i = 0; i < maps_entries; i++) {
    if (maps_arr[i].valid) {
      switch (maps_arr[i].entry_type) {
      case HEAP:
        num_heap_pages += maps_arr[i].present_pages;
        break;
      case ANON:
        num_anon_pages += maps_arr[i].present_pages;
        break;
      case LIBRARY:
        num_library_pages += maps_arr[i].present_pages;
        break;
      case STACK:
        num_stack_pages += maps_arr[i].present_pages;
        break;
      case OTHER:
        num_other_pages += maps_arr[i].present_pages;
        break;
      case FILEMAPPED:
        num_filemapped_pages += maps_arr[i].present_pages;
        break;
      default:
        printf("ERROR: Unknown type %d!\n", maps_arr[i].present_pages);
        assert(0);
      }
    }
  }
  free(maps_arr);

  return 0;
}
