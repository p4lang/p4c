/*
 * SPDX-FileCopyrightText: 2020 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control ingress(inout bit<8> h) {
    action a(inout bit<8> b) {
    }
    apply {
        bit<8> tmp = h;
        a(tmp);
        h = tmp;
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top(ingress()) main;
