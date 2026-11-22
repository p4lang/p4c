/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type intrinsic_metadata_t {
    fields {
        mcast_grp : 4;
        egress_rid : 4;
    }
}

header_type meta_t {
    fields {
        meter_tag : 32;
    }
}

metadata meta_t meta;

parser start {
    return parse_ethernet;
}

header ethernet_t ethernet;
metadata intrinsic_metadata_t intrinsic_metadata;

parser parse_ethernet {
    extract(ethernet);
    return ingress;
}

action _drop() {
    drop();
}

action _nop() {
}

meter my_meter {
    type: packets; // or bytes
    static: m_table;
    instance_count: 16384;
}

action m_action(meter_idx) {
    execute_meter(my_meter, meter_idx, meta.meter_tag);
    modify_field(standard_metadata.egress_spec, 1);
}

table m_table {
    reads {
        ethernet.srcAddr : exact;
    }
    actions {
        m_action; _nop;
    }
    size : 16384;
}

table m_filter {
    reads {
        meta.meter_tag : exact;
    }
    actions {
        _drop; _nop;
    }
    size: 16;
}

control ingress {
    apply(m_table);
    apply(m_filter);
}

control egress {
}
