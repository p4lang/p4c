#include <core.p4>
#include <v1model.p4>

struct metadata {
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".$start") state start {
        transition select((bit<32>)standard_metadata.instance_type) {
            32w0: start_0;
            32w1: start_e2e_mirrored;
            32w2: start_i2e_mirrored;
            default: noMatch;
        }
    }
    @name(".start") state start_0 {
        transition accept;
    }
    @packet_entry @name(".start_e2e_mirrored") state start_e2e_mirrored {
        packet.lookahead<bit<32>>();
        transition accept;
    }
    @packet_entry @name(".start_i2e_mirrored") state start_i2e_mirrored {
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".nop") action nop() {
    }
    @name(".exact") table exact_1 {
        actions = {
            nop();
            @defaultonly NoAction_0();
        }
        key = {
            standard_metadata.egress_spec: exact @name("standard_metadata.egress_spec") ;
        }
        default_action = NoAction_0();
    }
    apply {
        exact_1.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
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

