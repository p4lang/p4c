#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct addr_t {
    bit<8> f0;
    bit<8> f1;
    bit<8> f2;
    bit<8> f3;
    bit<8> f4;
    bit<8> f5;
}

header ethernet_t {
    bit<8>  _dstAddr_f00;
    bit<8>  _dstAddr_f11;
    bit<8>  _dstAddr_f22;
    bit<8>  _dstAddr_f33;
    bit<8>  _dstAddr_f44;
    bit<8>  _dstAddr_f55;
    bit<8>  _srcAddr_f06;
    bit<8>  _srcAddr_f17;
    bit<8>  _srcAddr_f28;
    bit<8>  _srcAddr_f39;
    bit<8>  _srcAddr_f410;
    bit<8>  _srcAddr_f511;
    bit<16> _etherType12;
}

struct headers {
    ethernet_t ethernet;
}

struct metadata_t {
}

parser MyParser(packet_in packet, out headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyIngress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    @name("MyIngress.x") bit<8> x_0;
    @hidden action nested_ifs_in_controlbmv2l58() {
        hdr.ethernet._srcAddr_f17 = 8w0xf0;
    }
    @hidden action nested_ifs_in_controlbmv2l61() {
        x_0 = hdr.ethernet._srcAddr_f17;
    }
    @hidden action nested_ifs_in_controlbmv2l63() {
        x_0 = hdr.ethernet._srcAddr_f17;
    }
    @hidden action nested_ifs_in_controlbmv2l56() {
        x_0 = 8w0;
    }
    @hidden action nested_ifs_in_controlbmv2l66() {
        hdr.ethernet._etherType12[7:0] = x_0;
    }
    @hidden action nested_ifs_in_controlbmv2l68() {
        standard_metadata.egress_spec = 9w1;
    }
    @hidden table tbl_nested_ifs_in_controlbmv2l56 {
        actions = {
            nested_ifs_in_controlbmv2l56();
        }
        const default_action = nested_ifs_in_controlbmv2l56();
    }
    @hidden table tbl_nested_ifs_in_controlbmv2l58 {
        actions = {
            nested_ifs_in_controlbmv2l58();
        }
        const default_action = nested_ifs_in_controlbmv2l58();
    }
    @hidden table tbl_nested_ifs_in_controlbmv2l61 {
        actions = {
            nested_ifs_in_controlbmv2l61();
        }
        const default_action = nested_ifs_in_controlbmv2l61();
    }
    @hidden table tbl_nested_ifs_in_controlbmv2l63 {
        actions = {
            nested_ifs_in_controlbmv2l63();
        }
        const default_action = nested_ifs_in_controlbmv2l63();
    }
    @hidden table tbl_nested_ifs_in_controlbmv2l66 {
        actions = {
            nested_ifs_in_controlbmv2l66();
        }
        const default_action = nested_ifs_in_controlbmv2l66();
    }
    @hidden table tbl_nested_ifs_in_controlbmv2l68 {
        actions = {
            nested_ifs_in_controlbmv2l68();
        }
        const default_action = nested_ifs_in_controlbmv2l68();
    }
    apply {
        if (hdr.ethernet._etherType12 == 16w0x22f0) {
            tbl_nested_ifs_in_controlbmv2l56.apply();
            if (hdr.ethernet._srcAddr_f28 == 8w0xa) {
                tbl_nested_ifs_in_controlbmv2l58.apply();
            } else if (hdr.ethernet._srcAddr_f28 == 8w0xb) {
                if (hdr.ethernet._srcAddr_f06 == 8w0xf0) {
                    tbl_nested_ifs_in_controlbmv2l61.apply();
                } else if (hdr.ethernet._srcAddr_f06 == 8w0xe0) {
                    tbl_nested_ifs_in_controlbmv2l63.apply();
                }
            }
            tbl_nested_ifs_in_controlbmv2l66.apply();
        }
        tbl_nested_ifs_in_controlbmv2l68.apply();
    }
}

control MyEgress(inout headers hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyComputeChecksum(inout headers hdr, inout metadata_t meta) {
    apply {
    }
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

V1Switch<headers, metadata_t>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
