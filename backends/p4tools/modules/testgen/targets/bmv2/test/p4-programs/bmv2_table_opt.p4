#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16>         ether_type;
}

header ipv4_t {
    bit<4>      version;
    bit<4>      ihl;
    bit<6>      dscp;
    bit<2>      ecn;
    bit<16>     total_len;
    bit<16>     identification;
    bit<1>      reserved;
    bit<1>      do_not_fragment;
    bit<1>      more_fragments;
    bit<13>     frag_offset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     header_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

struct local_metadata_t {
}

struct Headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}


parser p(packet_in pkt, out Headers h, inout local_metadata_t local_metadata, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(h.ethernet);
        pkt.extract(h.ipv4);
        transition accept;
    }

}

control vrfy(inout Headers h, inout local_metadata_t local_metadata) {
    apply { }
}


control ingress(inout Headers h, inout local_metadata_t local_metadata, inout standard_metadata_t s) {
    action acl_drop(inout standard_metadata_t standard_metadata) {
        mark_to_drop(standard_metadata);
    }

    table acl_ingress_table {
        key = {
            h.ethernet.dst_addr                       : exact @name("dst_mac");
            h.ipv4.src_addr                           : exact @name("src_ip");
            h.ipv4.dst_addr                           : exact @name("dst_ip");
            h.ipv4.ttl                                : optional @name("ttl");
            h.ipv4.dscp                               : optional @name("dscp");
            h.ipv4.ecn                                : optional @name("ecn");
            h.ipv4.protocol                           : optional @name("ip_protocol");
        }
        actions = {
            acl_drop(s);
            @defaultonly NoAction;
        }
        const default_action = NoAction;
    }
    apply {
        if (h.ipv4.isValid()) {
            acl_ingress_table.apply();
        }
    }

}

control egress(inout Headers h, inout local_metadata_t local_metadata, inout standard_metadata_t s) {
    apply { }
}

control update(inout Headers h, inout local_metadata_t local_metadata) {
    apply { }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}


V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;
