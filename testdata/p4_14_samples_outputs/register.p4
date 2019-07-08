#include <core.p4>
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<4> mcast_grp;
    bit<4> egress_rid;
}

struct meta_t {
    bit<32> register_tmp;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    @name(".meta") 
    meta_t meta;
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

@name(".my_register") register<bit<32>>(32w16384) my_register;

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".m_action") action m_action(bit<8> register_idx) {
        my_register.read(meta.meta.register_tmp, (bit<32>)register_idx);
    }
    @name("._nop") action _nop() {
    }
    @name(".m_table") table m_table {
        actions = {
            m_action;
            _nop;
        }
        key = {
            hdr.ethernet.srcAddr: exact;
        }
        size = 16384;
    }
    apply {
        m_table.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

