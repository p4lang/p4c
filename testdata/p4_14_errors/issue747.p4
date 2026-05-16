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
    return ingress;
}

// This test passes local_port (an untyped variable) to recirculate
// which expects a field list. It exposed issue #747, where the code
// expected that calling ::P4::error will exit out of the function.
action local_recirc(local_port) {
    resubmit( local_port );
}
table do_local_recirc {
    actions { local_recirc; }
}

control ingress {
  apply(do_local_recirc);
}


control egress { }
