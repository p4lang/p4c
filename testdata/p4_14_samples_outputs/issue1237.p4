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
        packet.extract(hdr.kilometer);
        packet.extract(hdr.expressivenesss);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".mooneys") action mooneys() {
        hdr.expressivenesss.breasted = hdr.expressivenesss.breasted - (bit<16>)hdr.expressivenesss.peptides;
        hdr.kilometer.sleeting = hdr.kilometer.sleeting - 16w7;
    }
    @name(".conceptualization") table conceptualization {
        actions = {
            mooneys;
        }
        key = {
            hdr.kilometer.isValid()      : exact;
            48w0                         : lpm @name("kilometer.flaccidly") ;
            hdr.expressivenesss.isValid(): exact;
        }
    }
    apply {
        conceptualization.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.kilometer);
        packet.emit(hdr.expressivenesss);
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

