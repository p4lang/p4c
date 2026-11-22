/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type foo_t {
    fields {
        src : 8;
        dst : 8;
    }
}

parser start {
    return parse_foo;
}

header foo_t foo;

parser parse_foo {
    extract(foo);
    return ingress;
}

action _drop() {
    drop();
}

action _nop() {
}

table forward {
    reads {
        foo.src : exact;
        foo.dst : exact;
    }
    actions {
        _drop;
	_nop;
    }
    size: 4;
}

control ingress {
    foobar();
}

control foobar {
    apply(forward) {
        hit { foobar(); }
    }
}

control egress { }
