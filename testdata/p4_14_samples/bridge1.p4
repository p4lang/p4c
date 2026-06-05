/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type data_t {
    fields {
        f1 : 32;
        f2 : 32;
        h1 : 16;
        b1 : 8;
        b2 : 8;
    }
}
header data_t data;

header_type metadata_t {
    fields {
        val : 8;
    }
}
metadata metadata_t meta;

parser start {
    extract(data);
    return ingress;
}

action noop() { }
action setb1(val, port) {
    modify_field(meta.val, val);
    modify_field(standard_metadata.egress_spec, port);
}

table test1 {
    reads {
        data.f1 : exact;
    }
    actions {
        setb1;
        noop;
    }
}

table test2 {
    reads { meta.val : exact; }
    actions { noop; }
}

control ingress {
    apply(test1);
    apply(test2);
}

action copyb1() {
    modify_field(data.b1, meta.val);
}

table output {
    actions { copyb1; }
    default_action: copyb1;
}

control egress {
    apply(output);
}

