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
 * This file contains general ebpf definitions.
 * It should be included with new target header files.
 */

#include <stdio.h>      // printf
#include <linux/bpf.h>  // types, and general bpf definitions
#include <stdbool.h>    // true and false

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;

#ifndef ___constant_swab16
#define ___constant_swab16(x) ((__u16)(             \
    (((__u16)(x) & (__u16)0x00ffU) << 8) |          \
    (((__u16)(x) & (__u16)0xff00U) >> 8)))
#endif

#ifndef ___constant_swab32
#define ___constant_swab32(x) ((__u32)(             \
    (((__u32)(x) & (__u32)0x000000ffUL) << 24) |        \
    (((__u32)(x) & (__u32)0x0000ff00UL) <<  8) |        \
    (((__u32)(x) & (__u32)0x00ff0000UL) >>  8) |        \
    (((__u32)(x) & (__u32)0xff000000UL) >> 24)))
#endif

#ifndef ___constant_swab64
#define ___constant_swab64(x) ((__u64)(             \
    (((__u64)(x) & (__u64)0x00000000000000ffULL) << 56) |   \
    (((__u64)(x) & (__u64)0x000000000000ff00ULL) << 40) |   \
    (((__u64)(x) & (__u64)0x0000000000ff0000ULL) << 24) |   \
    (((__u64)(x) & (__u64)0x00000000ff000000ULL) <<  8) |   \
    (((__u64)(x) & (__u64)0x000000ff00000000ULL) >>  8) |   \
    (((__u64)(x) & (__u64)0x0000ff0000000000ULL) >> 24) |   \
    (((__u64)(x) & (__u64)0x00ff000000000000ULL) >> 40) |   \
    (((__u64)(x) & (__u64)0xff00000000000000ULL) >> 56)))
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#ifndef __constant_htonll
#define __constant_htonll(x) (___constant_swab64((x)))
#endif

#ifndef __constant_ntohll
#define __constant_ntohll(x) (___constant_swab64((x)))
#endif

#define __constant_htonl(x) (___constant_swab32((x)))
#define __constant_ntohl(x) (___constant_swab32(x))
#define __constant_htons(x) (___constant_swab16((x)))
#define __constant_ntohs(x) ___constant_swab16((x))

#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# warning "I never tested BIG_ENDIAN machine!"
#define __constant_htonll(x) (x)
#define __constant_ntohll(X) (x)
#define __constant_htonl(x) (x)
#define __constant_ntohl(x) (x)
#define __constant_htons(x) (x)
#define __constant_ntohs(x) (x)

#else
# error "Fix your compiler's __BYTE_ORDER__?!"
#endif

#define htonl(d) __constant_htonl(d)
#define htons(d) __constant_htons(d)
#define htonll(d) __constant_htonll(d)

#define load_byte(data, b) (*(((u8*)(data)) + (b)))
#define load_half(data, b) __constant_ntohs(*(u16 *)((u8*)(data) + (b)))
#define load_word(data, b) __constant_ntohl(*(u32 *)((u8*)(data) + (b)))
#define load_dword(data, b) __constant_ntohll(*(u64 *)((u8*)(data) + (b)))