/*
Copyright 2020 Orange

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

#ifndef P4C_UBPF_TEST_H
#define P4C_UBPF_TEST_H

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>

#define MAX_PRINTF_LENGTH 80

/* simple descriptor which replaces the DPDK dp_packet structure */
struct dp_packet {
    void *data;          /* First byte actually in use. */
    uint32_t size_;      /* Number of bytes in use. */
};

static inline void ubpf_printf_test(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char str[MAX_PRINTF_LENGTH];
    if (vsnprintf(str, MAX_PRINTF_LENGTH, fmt, args) >= 0)
        printf("%s\n", str);
    va_end(args);
}

static inline void *ubpf_packet_data_test(void *ctx) {
    struct dp_packet *dp = (struct dp_packet *) ctx;
    return dp->data;
}

static inline void *ubpf_adjust_head_test(void *ctx, int offset) {
    struct dp_packet *dp = (struct dp_packet *) ctx;

    if (offset == 0) {
        return dp->data;
    } else if (offset > 0) {
        dp->data = realloc(dp->data, dp->size_ + offset);
        memcpy((char *) dp->data + offset, dp->data, dp->size_);
        dp->size_ += offset;
        return dp->data;
    } else {
        int ofs = abs(offset);
        memcpy(dp->data, (char *) dp->data + ofs, dp->size_ - ofs);
        dp->data = realloc(dp->data, dp->size_ - ofs);
        dp->size_ -= ofs;
        return dp->data;
    }
}

static inline uint32_t ubpf_truncate_packet_test(void *ctx, int maxlen)
{
    struct dp_packet *dp = (struct dp_packet *) ctx;
    uint32_t cutlen= 0;

    if (maxlen < 0)
        return 0;

    if (maxlen >= dp->size_) {
        cutlen = 0;
    } else {
        cutlen = dp->size_ - maxlen;
        dp->data = realloc(dp->data, maxlen);
        dp->size_ = maxlen;
    }

    return cutlen;
}


#endif //P4C_UBPF_TEST_H