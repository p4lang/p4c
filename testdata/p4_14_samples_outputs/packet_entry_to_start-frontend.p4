@name(".$InstanceType") enum bit<32> InstanceType_0 {
    START = 32w0,
    start_i2e_mirrored = 32w1
}

#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

@name("start") struct start_1 {
    bit<32> f1;
}

struct metadata {
    @name(".m") 
    start_1 m;
}

struct headers {
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start_0 {
        transition accept;
    }
    @packet_entry @name(".start_i2e_mirrored") state start_i2e_mirrored {
        transition start_0;
    }
    @name(".$start") state start {
        transition select((InstanceType_0)standard_metadata.instance_type) {
            InstanceType_0.START: start_0;
            InstanceType_0.start_i2e_mirrored: start_i2e_mirrored;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name(".a1") action a1() {
        meta.m.f1 = 32w1;
    }
    @name(".t1") table t1_0 {
        actions = {
            a1();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        t1_0.apply();
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

