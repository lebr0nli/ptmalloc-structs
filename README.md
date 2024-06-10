# ptmalloc-structs

Builds struct definitions for the glibc ptmalloc2 allocator for use in GDB.

## Building

```shell
$ # build for x86_64
$ make x86_64
$ # build for i386
$ make i386
$ # build for all supported architectures
$ make all
$ # clean up
$ make clean
```

The resulting files will be in the `build` directory.

## Usage

```shell
$ gdb /path/to/your/program
...
(gdb) add-symbol-file /path/to/ptmalloc-structs/ptmalloc-structs-<arch>-<glibc-version>.debug
...
(gdb) set $main_arena = *(struct malloc_state *) <address of main_arena>
(gdb) p $main_arena
```
