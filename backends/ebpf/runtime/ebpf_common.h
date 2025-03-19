/*
Copyright 2018 VMware, Inc.
SPDX-License-Identifier: (GPL-2.0-only or Apache-2.0)

This file is dual-license, so that when it is included from a source
file intended to be compiled and loaded into the Linux kernel, it can
be included from a file licensed under GPL-2.0-only.

When it is included from a source file intended to be compiled and run
as a user space program, it can be included from a file licensed as
Apache-2.0.

*/

/*
 * This file contains general type definitions used in ebpf programs.
 * It should be included with new target header files.
 */

#include <stdio.h>          // printf
#include <stdbool.h>        // true and false
#include <linux/types.h>    // u8, u16, u32, u64

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;


/// Helper function.
/// Print a byte buffer according to the specified length.
static inline void print_n_bytes(void *receiveBuffer, int num) {
    for (int i = 0; i < num; i++)
        printf("%02x", ((unsigned char *)receiveBuffer)[i]);
    printf("\n");
}
