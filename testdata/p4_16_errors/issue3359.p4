/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser mypt(in bit tt);
package mypack(mypt t);

parser MyParser(in bit tt) {
    state start {
        transition select(tt) {
            0: accept;
            _: reject;
        }
    }
}

parser MyParser1(in bit tt) {
    mypt t = MyParser();
    state start {
        t.apply(tt);
        transition select(tt) {
            0: accept;
            _: reject;
        }
    }
}

parser MyParser2(in bit tt) {
    mypt t;
    state start {
        t = MyParser();
        t.apply(tt);
        transition select(tt) {
            0: accept;
            _: reject;
        }
    }
}

mypack(MyParser1()) main;
