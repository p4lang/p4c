/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

header foo {
  bit<8> foo_1;
  bit<7> foo_2;
  bit<8> foo_3;
}

header bar {
  bit<2> bar_1;
}

struct baz {
  foo baz_1;
}

struct Headers {
    bar a;
    baz b;
}

struct Meta {}

parser p(packet_in b, out Headers h, inout Meta m, inout standard_metadata_t sm) {
    state start {
        transition accept;
    }
}

control vrfy(in Headers h, inout Meta m) { apply {} }
control update(inout Headers h, inout Meta m) { apply {} }

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }
control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) { apply {} }
control deparser(packet_out b, in Headers h) { apply {} }

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
