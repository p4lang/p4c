#include <core.p4>
#include <v1model.p4>

header len_or_type_t {
    bit<48> value;
}

header mac_da_t {
    bit<48> mac;
}

header mac_sa_t {
    bit<48> mac;
}

struct __metadataImpl {
    @name("standard_metadata") 
    standard_metadata_t standard_metadata;
}

struct __headersImpl {
    @name("len_or_type") 
    len_or_type_t len_or_type;
    @name("mac_da") 
    mac_da_t      mac_da;
    @name("mac_sa") 
    mac_sa_t      mac_sa;
}

parser __ParserImpl(packet_in packet, out __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".parse_len_or_type") state parse_len_or_type {
        packet.extract<len_or_type_t>(hdr.len_or_type);
        transition accept;
    }
    @name(".parse_mac_da") state parse_mac_da {
        packet.extract<mac_da_t>(hdr.mac_da);
        transition parse_mac_sa;
    }
    @name(".parse_mac_sa") state parse_mac_sa {
        packet.extract<mac_sa_t>(hdr.mac_sa);
        transition parse_len_or_type;
    }
    @name(".start") state start {
        transition parse_mac_da;
    }
}

control egress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".t2") table t2 {
        actions = {
            nop();
            @defaultonly NoAction();
        }
        key = {
            hdr.mac_sa.mac: exact @name("hdr.mac_sa.mac") ;
        }
        default_action = NoAction();
    }
    apply {
        t2.apply();
    }
}

control ingress(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".t1") table t1 {
        actions = {
            nop();
            @defaultonly NoAction();
        }
        key = {
            hdr.mac_da.mac       : exact @name("hdr.mac_da.mac") ;
            hdr.len_or_type.value: exact @name("hdr.len_or_type.value") ;
        }
        default_action = NoAction();
    }
    apply {
        t1.apply();
    }
}

control __egressImpl(inout __headersImpl hdr, inout __metadataImpl meta, inout standard_metadata_t __standard_metadata) {
    apply {
    }
}

control __DeparserImpl(packet_out packet, in __headersImpl hdr) {
    apply {
        packet.emit<mac_da_t>(hdr.mac_da);
        packet.emit<mac_sa_t>(hdr.mac_sa);
        packet.emit<len_or_type_t>(hdr.len_or_type);
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
