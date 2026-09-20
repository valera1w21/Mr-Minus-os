# Learning C

I am learning the C language by writing small Unix tools from scratch.

No tutorials copied line by line. I write the code, break it, read the compiler errors, and fix it.

## Why

I work with web development, but I wanted to understand what happens **below** the code I write every day. How a file is opened. What a pointer really is. Why a program crashes.

The long-term goal: build a minimal Linux system for an old phone.

## Tools I built

### `moy_cat` — my own `cat`

Reads files and prints them to the screen.

```
./moy_cat file.txt
./moy_cat file1.txt file2.txt file3.txt
```

What it does:

- checks that a filename was given
- opens each file in a loop
- reads it character by character until `EOF`
- if a file is missing, prints the real reason and continues with the next one
- returns exit code `1` if any file failed, `0` if all were fine

## What I learned so far

**Program structure**
- `#include` — the preprocessor pastes another file into mine
- `main` as the entry point
- `argc` / `argv` — how a program receives command line arguments
- exit codes: `0` means success, anything else means a problem

**Pointers**
- a pointer holds an address, not a value
- `&x` gives the address of `x`
- `*p = 5` writes to the address stored in `p`
- writing to address `0` gives `Segmentation fault`
- `NULL` means "points nowhere"

**Files**
- `fopen` / `fgetc` / `fclose`
- `EOF` is `-1`, so the character variable must be `int`, not `char`
- always check if `fopen` returned `NULL`

**Control flow**
- `if`, `while`, `for`
- `return` exits the function, `break` exits the loop, `continue` skips one step
- `=` assigns, `==` compares

**Patterns**
- early exit: handle bad cases first, keep the normal path flat
- error flag: keep working after a failure, but report it at the end

## Build

```
gcc moy_cat.c -o moy_cat
```

## Next

- `wc` — count lines, words and characters
- `grep` — print only lines containing a word
- `init` — the first process the kernel starts

## License

Public domain. Take anything you find useful.
