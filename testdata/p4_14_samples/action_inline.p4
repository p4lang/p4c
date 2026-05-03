/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


header_type ht { fields { b : 1; } }
metadata ht md;

parser start {
    return ingress;
}

action a(y0)
{ add_to_field(y0, 1); }
    
action b() {
    a(md.b);
    a(md.b);
}

table t {
    actions { b; }
}

control ingress {
    apply(t);
}
