#include "../extract_memory/read_memory.h"
#include "stdio.h"
#include <assert.h>
#include <string.h>

#define MAX_LEN_CMDLINE 75

uint64_t
get_file_size(FILE *f) {
    uint64_t size;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    rewind(f);

    return size;
}

int
split_binary(FILE *file_bin_in, FILE *file_type_in, FILE *file_filemapped_out,
             FILE *file_heap_out, FILE *file_anon_out, FILE *file_library_out,
             FILE *file_stack_out, FILE *file_other_out) {

    uint64_t size_type, size_bin, i;
    uint64_t num_type_pages, num_bin_pages;

    size_type = get_file_size(file_type_in);
    size_bin = get_file_size(file_bin_in);

    num_bin_pages = size_bin / PAGE_SIZE;
    num_type_pages = size_type / sizeof(entry_type);

    if (num_bin_pages != num_type_pages) {
        printf("Unexpected number of pages! num_bin_pages = %ld, "
               "num_type_pages = %ld\n",
               num_bin_pages, num_type_pages);
        assert(0);
    }

    printf("NUM PAGES: %ld\n", num_bin_pages);

    entry_type *types = malloc(num_type_pages * sizeof(entry_type));
    entry_type tmp;

    i = 0;

    while (fread(&tmp, 1, sizeof(entry_type), file_type_in) ==
           sizeof(entry_type)) {
        types[i] = tmp;
        i++;
    }

    int *page_buffer = (int *)malloc(PAGE_SIZE);
    i = 0;
    // Read input file
    while (fread(page_buffer, 1, PAGE_SIZE, file_bin_in) == PAGE_SIZE) {
        switch (types[i]) {
        case HEAP:
            fwrite(page_buffer, PAGE_SIZE, 1, file_heap_out);
            break;
        case ANON:
            fwrite(page_buffer, PAGE_SIZE, 1, file_anon_out);
            break;
        case LIBRARY:
            fwrite(page_buffer, PAGE_SIZE, 1, file_library_out);
            break;
        case STACK:
            fwrite(page_buffer, PAGE_SIZE, 1, file_stack_out);
            break;
        case OTHER:
            fwrite(page_buffer, PAGE_SIZE, 1, file_other_out);
            break;
        case FILEMAPPED:
            fwrite(page_buffer, PAGE_SIZE, 1, file_filemapped_out);
            break;
        default:
            printf("ERROR: Unknown type %d!\n", types[i]);
            assert(0);
        }

        i++;
    }

    free(page_buffer);
    free(types);

    return 0;
}

int
main(int argc, char **argv) {
    /* argv[1] - path to binary input file (original dump file with all data)
     * NOTE: The binary file needs to have all its data intact (that is,
     * created with memdum_pid.sh but WITH the option --ignorelimit
     * and WITHOUT the option --excludenull)
     * for the split_memdump to work properly.
     */

    char path_type_file_in[100] = "";

    char path_filemapped_out[100] = "";
    char path_heap_out[100] = "";
    char path_anon_out[100] = "";
    char path_library_out[100] = "";
    char path_stack_out[100] = "";
    char path_other_out[100] = "";

    FILE *file_bin_in;
    FILE *file_type_in;
    FILE *file_filemapped_out;
    FILE *file_heap_out;
    FILE *file_anon_out;
    FILE *file_library_out;
    FILE *file_stack_out;
    FILE *file_other_out;

    if (argc != 2) {
        printf("ERROR! Usage: ./split_memdump path/to/memdump\n");
        return 1;
    }

    for (int i = 0; i < strlen(argv[1]) - 4; i++) {
        path_filemapped_out[i] = argv[1][i];
        path_heap_out[i] = argv[1][i];
        path_anon_out[i] = argv[1][i];
        path_library_out[i] = argv[1][i];
        path_stack_out[i] = argv[1][i];
        path_other_out[i] = argv[1][i];
        path_type_file_in[i] = argv[1][i];
    }

    file_bin_in = fopen(argv[1], "rb");
    if (file_bin_in == NULL) {
        printf("ERROR reading file %s\n", argv[1]);
        return 1;
    }

    strcat(path_type_file_in, "_type.txt");
    file_type_in = fopen(path_type_file_in, "rb");
    if (file_type_in == NULL) {
        printf("ERROR reading file %s\n", path_type_file_in);
        return 1;
    }

    strcat(path_filemapped_out, "_filemapped.bin");
    remove(path_filemapped_out);
    file_filemapped_out = fopen(path_filemapped_out, "ab");
    if (file_filemapped_out == NULL) {
        printf("ERROR reading file %s\n", path_filemapped_out);
        return 1;
    }

    strcat(path_heap_out, "_heap.bin");
    remove(path_heap_out);
    file_heap_out = fopen(path_heap_out, "ab");
    if (file_heap_out == NULL) {
        printf("ERROR reading file %s\n", path_heap_out);
        return 1;
    }

    strcat(path_anon_out, "_anon.bin");
    remove(path_anon_out);
    file_anon_out = fopen(path_anon_out, "ab");
    if (file_anon_out == NULL) {
        printf("ERROR reading file %s\n", path_anon_out);
        return 1;
    }

    strcat(path_library_out, "_library.bin");
    remove(path_library_out);
    file_library_out = fopen(path_library_out, "ab");
    if (file_library_out == NULL) {
        printf("ERROR reading file %s\n", path_library_out);
        return 1;
    }

    strcat(path_stack_out, "_stack.bin");
    remove(path_stack_out);
    file_stack_out = fopen(path_stack_out, "ab");
    if (file_stack_out == NULL) {
        printf("ERROR reading file %s\n", path_stack_out);
        return 1;
    }

    strcat(path_other_out, "_other.bin");
    remove(path_other_out);
    file_other_out = fopen(path_other_out, "ab");
    if (file_other_out == NULL) {
        printf("ERROR reading file %s\n", path_other_out);
        return 1;
    }

    split_binary(file_bin_in, file_type_in, file_filemapped_out, file_heap_out,
                 file_anon_out, file_library_out, file_stack_out,
                 file_other_out);

    fclose(file_bin_in);
    fclose(file_type_in);
    fclose(file_filemapped_out);
    fclose(file_heap_out);
    fclose(file_anon_out);
    fclose(file_library_out);
    fclose(file_stack_out);
    fclose(file_other_out);

    return 0;
}
