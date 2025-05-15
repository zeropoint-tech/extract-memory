#include "read_memory.h"
#include <string.h>

#define MAX_LEN_CMDLINE 75

int main(int argc, char **argv) {
  /* argv[1] - pid of process to memory dump
   * argv[2] - path to binary output file (dump file)
   * the application will print data (can/should be redirected to txt file)
   */

  if (argc != 3) {
    printf("Usage: ./memdump <pid> <path/to/output/file>\n");
    return 1;
  }

  char ch, path_proc_cmd[100];
  char path_maps_file_out[100] = "";
  char path_type_file_out[100] = "";
  int pid, max_ch_cmd_line, status = 0;
  FILE *file_proc_cmd, *file_bin_out, *file_maps_out, *file_type_out;

  /* delete previous binary dump file */
  remove(argv[2]);

  pid = atoi(argv[1]);
  file_bin_out = fopen(argv[2], "ab");

  for (int i = 0; i < strlen(argv[2]) - 4; i++) {
    path_maps_file_out[i] = argv[2][i];
    path_type_file_out[i] = argv[2][i];
  }

  strcat(path_maps_file_out, "_maps.txt");
  file_maps_out = fopen(path_maps_file_out, "w");

  strcat(path_type_file_out, "_type.txt");
  file_type_out = fopen(path_type_file_out, "w");

  sprintf(path_proc_cmd, "/proc/%u/cmdline", pid);
  file_proc_cmd = fopen(path_proc_cmd, "r");

  if (file_proc_cmd == NULL) {
    printf("Error: opening the /proc/pid/cmdline file.\n");
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
