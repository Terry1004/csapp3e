# Overview

This repo contains my solutions for labs in [CSAPP3e](https://csapp.cs.cmu.edu/3e/labs.html) except the Performance lab (I did the Cache lab instead). However, do note that I may **NOT** follow the instructions exactly. I also have edited some compilation flags for the solution to work properly on the more recent `gcc`, and some `Makefile` commands to clean more files.

# Environment
I develop the solutions on M1 mac, so I make no guarantee the solutions could work on other machines. Even though I use docker to make the solutions as portable as possible, some of these labs involve low-level systems which could be broken depending on the implementation and configuration of the docker engine.

# Usage
The entry point to compile and run any solution is ./run.sh. Some examples are as follows:
```sh
$ ./run.sh datalab
```
```sh
$ ./run.sh cachelab csim
```
```sh
$ ./run.sh proxylab debug
```

# Notes
## Run `i386` Programs
Data Lab, Bomb Lab and Attack Lab require running `i386` programs. To achieve this on M1 mac, do the following:
1. the current `docker buildx ls` context needs to support both `amd64` and `i386` architectures
2. use amd64 architecture for docker virtual machine -- I am using `colima` and I use the following command to start:
   ```sh
   $ colima start --arch amd64
   ```

## Run `gdb`
Bomb Lab and Attack Lab require running `gdb` for `i386` programs. The solution can be found [here](https://gist.github.com/securisec/b88cf9e89f957669b95043c9c380a26e):
1. Start the docker container:
   ```sh
   $ ./run.sh bomb debug
   ```
   or
   ```sh
   $ ./run.sh target1 debug
   ```
2. Enter the docker container:
   ```sh
   $ docker exec -it csapp3e bash
   ```
3. Go to the respective directory:
   ```sh
   $ cd bomb-handout
   ```
   or
   ```sh
   $ cd target1
   ```
4. Run the following to start `gdb`:
   ```sh
   $ ROSETTA_DEBUGSERVER_PORT=1234 ./bomb < psol.txt & gdb

   (gdb) set architecture i386:x86-64
   (gdb) file bomb
   (gdb) target remote localhost:1234
   ```
   or (e.g. for task `ctarget`)
   ```sh
   $ ./hex2raw < ./inputs/ctarget.1.txt | ROSETTA_DEBUGSERVER_PORT=1234 ./ctarget -q & gdb

   (gdb) set architecture i386:x86-64
   (gdb) file ctarget
   (gdb) target remote localhost:1234
   ```

## (SPOILER) Bomb Lab
1. The string is statically stored.
2. The program reads 6 numbers:
   1. The first number must be 1
   2. Each number must be twice of the previous one
3. The program reads 2 numbers. Use a jump table based on the first number to find the second number.
4. The program reads 2 numbers:
   1. The code does a binary search, but any number between 0 and 14 is accepted for the first one.
   2. The second number should be 0.
5. The program contains a constant string for encoding. It reads each character, mask it with `oxf`, and fetch the character from the constant at that index.
6. The program reads 6 numbers:
   1. Change each to `7-x`
   2. Find the node in a linked list at that index
   3. Reorder the linked list and check that the values stored there are in descending order

## Shell Lab
The test program will hang on the `trace08` test case. Therefore I add 10s timeout to shlab driver.

## Malloc Lab
Because running `i386` programs on M1 mac requires `qemu`, which significantly affects performance of the malloc performance, I decided to write and test the malloc function for `amd64` instead. I therefore made some changes to the `Makefile`.

Trace files for Malloc lab are not included in the files downloaded from [CSAPP3e](https://csapp.cs.cmu.edu/3e/labs.html). I downloaded them from [lsw8075/malloc-lab](https://github.com/lsw8075/malloc-lab).

