/*
 * SPDX-FileCopyrightText: 2020 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c(inout bit<8> val)(int a) {
    apply {
       val = (bit<8>) a;
    }
}

control ingress(inout bit<8> a) {
    c(0) x;
    apply {
        x.apply(a);
    }
}

control i(inout bit<8> a);
package top(i _i);

top(ingress()) main;
