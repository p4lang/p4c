#include "tofino/intrinsic_metadata.p4"

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type ipv4_t {
    fields {
        version : 4;
        ihl : 4;
        diffserv : 8;
        totalLen : 16;
        identification : 16;
        flags : 3;
        fragOffset : 13;
        ttl : 8;
        protocol : 8;
        hdrChecksum : 16;
        srcAddr : 32;
        dstAddr: 32;
    }
}

header_type tcp_t {
    fields {
        srcPort : 16;
        dstPort : 16;
        seqNo : 32;
        ackNo : 32;
        dataOffset : 4;
        res : 4;
        flags : 8;
        window : 16;
        checksum : 16;
        urgentPtr : 16;
    }
}


header_type meta_t {
     fields {
         pad_0 : 4;
         vrf : 12;
     }
}


header ethernet_t ethernet;
header ipv4_t ipv4;
header tcp_t tcp;
metadata meta_t meta;


parser start {
    return parse_ethernet;
}


parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        0x0800 : parse_ipv4;
        default : ingress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return select(ipv4.protocol){
       0x06 : parse_tcp;
       default : ingress;
    }
}

parser parse_tcp {
    extract(tcp);
    return ingress;
}

field_list ipv4_field_list {
    ipv4.version;
    ipv4.ihl;
    ipv4.diffserv;
    ipv4.totalLen;
    ipv4.identification;
    ipv4.flags;
    ipv4.fragOffset;
    ipv4.ttl;
    ipv4.protocol;
    ipv4.srcAddr;
    ipv4.dstAddr;
}

field_list_calculation ipv4_chksum_calc {
    input {
        ipv4_field_list;
    }
    algorithm : csum16;
    output_width: 16;
}

calculated_field ipv4.hdrChecksum {
    update ipv4_chksum_calc;
}

action hop(ttl, egress_port) {
    add_to_field(ttl, -1);
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
}

action ipv4_lpm_hit(egress_port) {
    hop(ipv4.ttl, egress_port);
}

action lpm_miss(){
    drop();
}

action nop() {}

@pragma alpm 1
table ipv4_alpm {
    reads {
        meta.vrf: exact;
        ipv4.dstAddr: lpm;
    }
    actions {
        ipv4_lpm_hit;
        lpm_miss;
        nop;
    }
    size: 8192;
}

@pragma alpm 1
@pragma alpm_partitions 1024
@pragma alpm_subtrees_per_partition 4
table ipv4_alpm_large {
    reads {
        meta.vrf: exact;
        ipv4.dstAddr: lpm;
    }
    actions {
        ipv4_lpm_hit;
        lpm_miss;
        nop;
    }
    size: 200000;
}

@pragma alpm 1
table ipv4_alpm_idle {
    reads {
        ipv4.dstAddr: lpm;
    }
    actions {
        ipv4_lpm_hit;
        nop;
    }
    size: 8192;
    support_timeout: true;
}

/* Main control flow */
control ingress {
    apply(ipv4_alpm);
    apply(ipv4_alpm_large);
    apply(ipv4_alpm_idle);
}
