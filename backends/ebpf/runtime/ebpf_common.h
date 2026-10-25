/*
 * Copyright 2018 VMware, Inc.
 * SPDX-License-Identifier: GPL-2.0-only or BSD-2-Clause
 * 
 * See this article for details on the dual license used for this
 * file:
 * https://github.com/p4lang/.github/blob/main/copyright-and-license-issues/licenses.md
 *
 */

/*
 * This file contains general type definitions used in ebpf programs.
 * It should be included with new target header files.
 */

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
