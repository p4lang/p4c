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
        packet.extract(hdr.mac_da);
        packet.extract(hdr.mac_sa);
        packet.extract(hdr.len_or_type);
        transition accept;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".nop") action nop() {
    }
    @name(".t2") table t2 {
        actions = {
            nop;
        }
        key = {
            hdr.mac_sa.mac: exact;
        }
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
            nop;
        }
        key = {
            hdr.mac_da.mac       : exact;
            hdr.len_or_type.value: exact;
        }
    }
    apply {
        t1.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.mac_da);
        packet.emit(hdr.mac_sa);
        packet.emit(hdr.len_or_type);
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

