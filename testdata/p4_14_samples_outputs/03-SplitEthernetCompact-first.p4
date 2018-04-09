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

struct metadata {
}

struct headers {
    @name(".len_or_type") 
    len_or_type_t len_or_type;
    @name(".mac_da") 
    mac_da_t      mac_da;
    @name(".mac_sa") 
    mac_sa_t      mac_sa;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<mac_da_t>(hdr.mac_da);
        packet.extract<mac_sa_t>(hdr.mac_sa);
        packet.extract<len_or_type_t>(hdr.len_or_type);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".t2") table t2 {
        actions = {
            nop();
            @defaultonly NoAction();
        }
        key = {
            hdr.mac_sa.mac: exact @name("mac_sa.mac") ;
        }
        default_action = NoAction();
    }
    apply {
        t2.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".t1") table t1 {
        actions = {
            nop();
            @defaultonly NoAction();
        }
        key = {
            hdr.mac_da.mac       : exact @name("mac_da.mac") ;
            hdr.len_or_type.value: exact @name("len_or_type.value") ;
        }
        default_action = NoAction();
    }
    apply {
        t1.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<mac_da_t>(hdr.mac_da);
        packet.emit<mac_sa_t>(hdr.mac_sa);
        packet.emit<len_or_type_t>(hdr.len_or_type);
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

