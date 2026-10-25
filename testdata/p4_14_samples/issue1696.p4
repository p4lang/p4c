/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type stack_t {
    fields {
        f : 16;
    }
}

header stack_t stack[4];

parser start {
    extract(stack[next]);
    return select(latest.f) {
        0xffff : start;
        default: ingress;
    }
}

control ingress { }
