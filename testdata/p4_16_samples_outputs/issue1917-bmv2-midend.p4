#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<48> macAddr_t;
typedef bit<32> ip4Addr_t;
header ethernet_t {
    macAddr_t dstAddr;
    macAddr_t srcAddr;
    bit<16>   etherType;
}

header ipv4_t {
    bit<4>    version;
    bit<4>    ihl;
    bit<8>    tos;
    bit<16>   totalLen;
    bit<16>   identification;
    bit<3>    flags;
    bit<13>   fragOffset;
    bit<8>    ttl;
    bit<8>    protocol;
    bit<16>   hdrChecksum;
    ip4Addr_t srcAddr;
    ip4Addr_t dstAddr;
}

struct count_struct_t {
    bit<8> counter;
}

struct metadata {
    bit<8> _count_struct_counter0;
}

struct headers {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

struct h {
    ethernet_t _hdr_ethernet0;
    ipv4_t     _hdr_ipv41;
}

parser MyParser(packet_in packet, out h hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr._hdr_ethernet0);
        transition select(hdr._hdr_ethernet0.etherType) {
            16w0x800: ipv4;
            default: accept;
        }
    }
    state ipv4 {
        packet.extract<ipv4_t>(hdr._hdr_ipv41);
        transition accept;
    }
}

control MyDeparser(packet_out packet, in h hdr) {
    apply {
        packet.emit<ethernet_t>(hdr._hdr_ethernet0);
        packet.emit<ipv4_t>(hdr._hdr_ipv41);
    }
}

control MyVerifyChecksum(inout h hdr, inout metadata meta) {
    apply {
    }
}

control MyIngress(inout h hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @hidden action issue1917bmv2l104() {
        standard_metadata.egress_spec = 9w2;
        standard_metadata.ingress_port = (bit<9>)standard_metadata.enq_timestamp;
    }
    @hidden table tbl_issue1917bmv2l104 {
        actions = {
            issue1917bmv2l104();
        }
        const default_action = issue1917bmv2l104();
    }
    apply {
        tbl_issue1917bmv2l104.apply();
    }
}

control MyEgress(inout h hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @hidden action issue1917bmv2l117() {
        standard_metadata.deq_qdepth = 19w1;
        standard_metadata.egress_spec = 9w0;
    }
    @hidden table tbl_issue1917bmv2l117 {
        actions = {
            issue1917bmv2l117();
        }
        const default_action = issue1917bmv2l117();
    }
    apply {
        tbl_issue1917bmv2l117.apply();
    }
}

control MyComputeChecksum(inout h hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<h, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;

