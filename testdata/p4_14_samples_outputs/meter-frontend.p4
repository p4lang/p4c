#include <core.p4>
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<4>  mcast_grp;
    bit<4>  egress_rid;
    bit<16> mcast_hash;
    bit<32> lf_field_list;
}

struct meta_t {
    bit<32> meter_tag;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    @name(".intrinsic_metadata") 
    intrinsic_metadata_t intrinsic_metadata;
    @name(".meta") 
    meta_t               meta;
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
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

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".my_meter") meter(32w16384, MeterType.packets) my_meter_0;
    @name("._drop") action _drop_0() {
        mark_to_drop();
    }
    @name("._nop") action _nop_0() {
    }
    @name(".m_action") action m_action_0(bit<32> meter_idx) {
        my_meter_0.execute_meter<bit<32>>(meter_idx, meta.meta.meter_tag);
        standard_metadata.egress_spec = 9w1;
    }
    @name(".m_filter") table m_filter_0 {
        actions = {
            _drop_0();
            _nop_0();
            @defaultonly NoAction();
        }
        key = {
            meta.meta.meter_tag: exact @name("meta.meter_tag") ;
        }
        size = 16;
        default_action = NoAction();
    }
    @name(".m_table") table m_table_0 {
        actions = {
            m_action_0();
            _nop_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.ethernet.srcAddr: exact @name("ethernet.srcAddr") ;
        }
        size = 16384;
        default_action = NoAction();
    }
    apply {
        m_table_0.apply();
        m_filter_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

