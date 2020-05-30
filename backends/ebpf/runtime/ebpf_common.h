/*
Copyright 2018 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/*
 * This file contains general type definitions used in ebpf programs.
 * It should be included with new target header files.
 */

#include <stdio.h>          // printf
#include <stdbool.h>        // true and false
#include <linux/types.h>    // u8, u16, u32, u64

#include "bpf_endian.h" // definitions for bpf_ntohs etc...

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;


#undef htonl
#undef htons
#define htons(d) __bpf_htons(d)
#define htonl(d) __bpf_ntohl(d)
#define htonll(d) __bpf_constant_be64_to_cpu(d)

#define load_byte(data, b) (*(((u8*)(data)) + (b)))
#define load_half(data, b) bpf_ntohs(*(u16 *)((u8*)(data) + (b)))
#define load_word(data, b) bpf_ntohl(*(u32 *)((u8*)(data) + (b)))
#define load_dword(data, b) bpf_cpu_to_be64(*(u64 *)((u8*)(data) + (b)))

/*
 * Helper function.
 * Print a byte buffer according to the specified length.
 */
static inline void print_n_bytes(void *receiveBuffer, int num) {
    for (int i = 0; i < num; i++)
        printf("%02x", ((unsigned char *)receiveBuffer)[i]);
    printf("\n");
}
