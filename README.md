# Memory dumper

This small tool takes a snapshot of the memory allocated for a specific process
(PID) and dumps it into a binary file. Optionally the file can be split into
separate binary files per each memory type.

## Memdumper

The main application is compiled from `dump_proc_memory.c`, which takes only 2
arguments:

- The PID of the application to dump memory from
- The output file for the binary, dumped data

> [!NOTE]
> The application also prints a lot of information and you might want to
> redirect that to a separate file as well.

Since the `memdump` application only takes snapshots of the used memory, e.g.
dumping `/proc/<pid>/maps`, it would have to be run multiple times in sequence
in order to see usage over time.

## Dump splitter

The `memdump` application creates a single, binary file output which in turn
has the different memory allocation types inside. If it is preferable to have
these as separate files instead, you can run `split_dump` on the binary. This
will create one file per memory type used, e.g., HEAP, STACK etc.

## Building

Build the two applications simply by running:

```sh
make
```

## Resulting files

The resulting files are raw memory dumps of all memory that is accessible from
the given process. If more information about the process is required, check out
[`gcore`](https://man7.org/linux/man-pages/man1/gcore.1.html).
