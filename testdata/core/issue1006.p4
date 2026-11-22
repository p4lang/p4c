/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern R<T> {
    R(T init);
};

struct foo {
    bit<8>      field1;
}


control c();
package top(c _c);

control c1() {
    R<tuple<bit<8>>>({ 1 }) reg0;
    R<foo>({ 1 }) reg1;
    apply {}
}

top(c1()) main;
