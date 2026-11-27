// SPDX-FileCopyrightText: 2026 Mojatatu Networks
//
// SPDX-License-Identifier: Apache-2.0

/* Parse template values with iproute2's own type table.
 *
 * The caller feeds "ptype<TAB>token" lines on stdin and gets back either
 * "reject" or "accept<TAB>valuebytes<TAB>maskbytes". Both the verdict and the
 * bytes are reported, because a parser can accept a token and put the wrong
 * value in the buffer.
 *
 * The parse mirrors what tc does for a table key: the token may carry a
 * "value/mask" suffix, which the type's own parse function splits.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/p4tc.h>

#include "p4_types.h"

/* The parse functions write the C container the type is stored in: a bit8
 * value is written through a __u8, a bit24 value through a __u32, and a bool
 * is written through a __u32 although the type is one bit wide. A key is
 * allocated at ceil(bitsz / 8) bytes, so the buffer the parser writes through
 * is not the buffer the key occupies. Reserve a full container behind the
 * bytes that are printed so the write stays inside the allocation.
 */
#define CONTAINER_BYTES 16

static void print_bytes(const unsigned char *bytes, size_t size) {
    size_t i;

    for (i = 0; i < size; i++) printf("%02x", bytes[i]);
}

int main(void) {
    char line[1024];

    register_p4_types();

    while (fgets(line, sizeof(line), stdin)) {
        struct p4_type_value val;
        struct p4_type_s *type;
        unsigned char *mask;
        unsigned char *value;
        char *separator;
        char *ptype;
        char *token;
        __u32 bitsz = 0;
        size_t bytesz;

        line[strcspn(line, "\n")] = 0;
        separator = strchr(line, '\t');
        if (!separator) continue;
        *separator = 0;
        ptype = line;
        token = separator + 1;

        type = get_p4type_byarg(ptype, &bitsz);
        if (!type) {
            printf("error\tunknown type %s\n", ptype);
            continue;
        }

        bytesz = (bitsz % 8) ? bitsz / 8 + 1 : bitsz / 8;
        value = calloc(1, bytesz + CONTAINER_BYTES);
        mask = calloc(1, bytesz + CONTAINER_BYTES);
        if (!value || !mask) {
            printf("error\tout of memory\n");
            free(value);
            free(mask);
            continue;
        }

        val.value = value;
        val.mask = mask;
        val.bitsz = bitsz ? bitsz : type->bitsz;
        if (!type->parse_p4t || type->parse_p4t(&val, token, 0) < 0) {
            printf("reject\n");
        } else {
            printf("accept\t");
            print_bytes(value, bytesz);
            printf("\t");
            print_bytes(mask, bytesz);
            printf("\n");
        }

        free(value);
        free(mask);
    }

    return 0;
}
