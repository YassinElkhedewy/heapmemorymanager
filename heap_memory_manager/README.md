# Heap Memory Manager Skeleton

This project provides replacements for the C library `malloc`, `free`, `calloc`, and `realloc` functions. The wrappers in `src/heapWrappers.c` are complete, but the functions in `src/HmmAlloc.c` and `src/HmmFree.c` are placeholders that must be implemented.

## Requirements

The project is intended for Linux because the allocator uses Linux/POSIX interfaces such as `sbrk`.

Install the required tools on Debian or Ubuntu:

```sh
sudo apt update
sudo apt install build-essential
```

## Implement the Allocator

Implement the functions declared in `inc/Hmm.h`:

- `HmmAlloc` in `src/HmmAlloc.c`
- `hmmCalloc` in `src/HmmAlloc.c`
- `hmmReAlloc` in `src/HmmAlloc.c`
- `hmmSize` in `src/HmmAlloc.c`
- `HmmFree` in `src/HmmFree.c`

Do not change `src/heapWrappers.c`. It redirects the standard allocation function names to the functions above.

## Build

From this directory, build the static library and test executable:

```sh
make
```

Build individual outputs:

```sh
make static_lib
make shared_lib
make test
```

The generated files are:

- `build/lib/libhmm.a`: static library
- `build/lib/libhmm.so`: shared library for Linux runtime interposition
- `build/hmm_test`: allocator test executable

Remove generated files with:

```sh
make clean
```

## Run the Tests

Build and run the test executable:

```sh
make test
./build/hmm_test
```

The provided placeholder functions return allocation failures, so the tests are expected to fail until the allocator is implemented.

For additional compiler diagnostics, override `CFLAGS` when building:

```sh
make clean
make CFLAGS="-Wall -Wextra -Wpedantic -g -fPIC -fsanitize=address,undefined"
./build/hmm_test
```

## Link an Application Statically

Build the static library, then place the library after the application's source or object files in the compiler command:

```sh
make static_lib
gcc -Iinc example.c build/lib/libhmm.a -o example
./example
```

The linked executable uses the wrappers from this project for its standard allocation calls.

## Use the Allocator in a Linux Shell

Linux can load `libhmm.so` before libc by using `LD_PRELOAD`. This redirects allocation calls made by dynamically linked programs to this library.

First, fully implement and test the allocator. Do not preload the current placeholder implementation because every allocation returns `NULL` and programs may fail to start.

Build the shared library and save its absolute path:

```sh
make shared_lib
export HMM_LIB="$(pwd)/build/lib/libhmm.so"
```

Use the allocator for one command:

```sh
LD_PRELOAD="$HMM_LIB" command_name argument1 argument2
```

For example:

```sh
LD_PRELOAD="$HMM_LIB" ls -la
```

To use the allocator for a temporary child shell and the dynamically linked commands launched from it:

```sh
LD_PRELOAD="$HMM_LIB" bash --noprofile --norc
```

Inside that child shell, confirm the setting and run commands normally:

```sh
printf '%s\n' "$LD_PRELOAD"
ls
grep --version
```

Leave the temporary shell with:

```sh
exit
```

If the current shell remains usable but a preloaded program fails, disable interposition with the shell builtin:

```sh
unset LD_PRELOAD
```

Do not add this library to `/etc/ld.so.preload` or a login profile. A defect in a globally preloaded memory allocator can prevent shells and recovery commands from starting.

`LD_PRELOAD` does not affect statically linked executables, secure/set-user-ID programs that reject preload variables, shell builtins already running in the parent shell, or programs that bypass the standard allocation functions.