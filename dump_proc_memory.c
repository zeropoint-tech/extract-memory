#include "read_memory.h"
#include <string.h>

#define MAX_LEN_CMDLINE 75
#define MAX_FILE_NAME_LEN 100

void usage() {
  printf("Usage: ./memdump <pid> <path>\n");
  printf("Params:\n"
         "        pid:  PID of the process to dump memory of\n"
         "        path: Path to binary output file - will overwrite if file "
         "already exists\n");
}

int main(int argc, char **argv) {
  if (argc != 3) {
    usage();
    return 1;
  }

  int ch, max_ch_cmd_line, status = 0;
  FILE *file_proc_cmd, *file_bin_out, *file_maps_out, *file_type_out;

  char *bin_file = argv[2];
  int pid = atoi(argv[1]);

  if (access(bin_file, F_OK) == 0) {
    printf("Overwriting existing bin file: %s\n", bin_file);
  }

  file_bin_out = fopen(bin_file, "wb");

  char path_maps_file_out[MAX_FILE_NAME_LEN] = "";
  char path_type_file_out[MAX_FILE_NAME_LEN] = "";
  for (int i = 0; i < strlen(bin_file) - 4; i++) {
    path_maps_file_out[i] = bin_file[i];
    path_type_file_out[i] = bin_file[i];
  }

  strcat(path_maps_file_out, "_maps.txt");
  file_maps_out = fopen(path_maps_file_out, "w");

  strcat(path_type_file_out, "_type.txt");
  file_type_out = fopen(path_type_file_out, "w");

  char path_proc_cmd[MAX_FILE_NAME_LEN];
  sprintf(path_proc_cmd, "/proc/%u/cmdline", pid);
  file_proc_cmd = fopen(path_proc_cmd, "r");

  if (file_proc_cmd == NULL) {
    printf("Error: opening the /proc/pid/cmdline file for PID %d.\n", pid);
    status = ENOENT;
    goto tear_down;
  }

  max_ch_cmd_line = MAX_LEN_CMDLINE;
  while ((ch = fgetc(file_proc_cmd)) != EOF && max_ch_cmd_line) {
    max_ch_cmd_line--;

    if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_' ||
        ch == '-' || ch == '/' || ch == '.' || ch == ',')
      printf("%c", ch);
  }
  printf("\n");

  fclose(file_proc_cmd);

  status = read_maps_file(pid, file_bin_out, file_maps_out, file_type_out);

tear_down:
  fclose(file_maps_out);
  fclose(file_type_out);
  fclose(file_bin_out);

  return status;
}
