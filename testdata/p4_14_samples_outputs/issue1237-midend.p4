#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

@name("backs") header backs_0 {
    bit<16> sharps;
    bit<16> peptides;
    bit<16> whirs;
    bit<16> breasted;
    bit<16> browning;
    bit<32> jews;
}

@name("affairs") header affairs_0 {
    bit<48> flaccidly;
    bit<16> sleeting;
}

struct metadata {
}

struct headers {
    @name(".expressivenesss")
    backs_0   expressivenesss;
    @name(".kilometer")
    affairs_0 kilometer;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<affairs_0>(hdr.kilometer);
        packet.extract<backs_0>(hdr.expressivenesss);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<48> key_0;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name(".mooneys") action mooneys() {
        hdr.expressivenesss.breasted = hdr.expressivenesss.breasted - hdr.expressivenesss.peptides;
        hdr.kilometer.sleeting = hdr.kilometer.sleeting + 16w65529;
    }
    @name(".conceptualization") table conceptualization_0 {
        actions = {
            mooneys();
            @defaultonly NoAction_1();
        }
        key = {
            hdr.kilometer.isValid()      : exact @name("kilometer.$valid$");
            key_0                        : lpm @name("kilometer.flaccidly");
            hdr.expressivenesss.isValid(): exact @name("expressivenesss.$valid$");
        }
        default_action = NoAction_1();
    }
    @hidden action issue1237l37() {
        key_0 = 48w0;
    }
    @hidden table tbl_issue1237l37 {
        actions = {
            issue1237l37();
        }
        const default_action = issue1237l37();
    }
    apply {
        tbl_issue1237l37.apply();
        conceptualization_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<affairs_0>(hdr.kilometer);
        packet.emit<backs_0>(hdr.expressivenesss);
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
