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

struct __metadataImpl {
    @name("intrinsic_metadata") 
    intrinsic_metadata_t intrinsic_metadata;
    @name("meta") 
    meta_t               meta;
    @name("standard_metadata") 
    standard_metadata_t  standard_metadata;
}

struct __headersImpl {
    @name("ethernet") 
    ethernet_t ethernet;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name("NoAction") action NoAction_0() {
    }
    @name("NoAction") action NoAction_3() {
    }
    @name(".my_meter") meter(32w16384, MeterType.packets) my_meter;
    @name("._drop") action _drop_0() {
        mark_to_drop();
    }
    @name("._nop") action _nop_0() {
    }
    @name("._nop") action _nop_2() {
    }
    @name(".m_action") action m_action_0(bit<32> meter_idx) {
        my_meter.execute_meter<bit<32>>(meter_idx, meta.meta.meter_tag);
        meta.standard_metadata.egress_spec = 9w1;
    }
    @name(".m_filter") table m_filter {
        actions = {
            _drop_0();
            _nop_0();
            @defaultonly NoAction_0();
        }
        key = {
            meta.meta.meter_tag: exact @name("meta.meta.meter_tag") ;
        }
        size = 16;
        default_action = NoAction_0();
    }
    @name(".m_table") table m_table {
        actions = {
            m_action_0();
            _nop_2();
            @defaultonly NoAction_3();
        }
        key = {
            hdr.ethernet.srcAddr: exact @name("hdr.ethernet.srcAddr") ;
        }
        size = 16384;
        default_action = NoAction_3();
    }
    apply {
        m_table.apply();
        m_filter.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
    }
}

control __verifyChecksumImpl(in __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

control __computeChecksumImpl(inout __headersImpl hdr, inout __metadataImpl meta) {
    apply {
    }
}

V1Switch<__headersImpl, __metadataImpl>(__ParserImpl(), __verifyChecksumImpl(), ingress(), __egressImpl(), __computeChecksumImpl(), __DeparserImpl()) main;
