#include <core.p4>
#include <v1model.p4>

header data_t {
    bit<32> f1;
    bit<32> f2;
    bit<32> f3;
    bit<32> f4;
    bit<8>  b1;
    bit<8>  b2;
    bit<16> h1;
}

struct metadata {
}

struct headers {
    @name(".data") 
    data_t data;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".start") state start {
        packet.extract<data_t>(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".copy2") action copy2_0(inout bit<8> dest_0, bit<8> val_0) {
        dest_0 = val_0;
    }
    @name(".copy") action copy_0(inout bit<8> dest_1, bit<8> val_1) {
        copy2_0(dest_1, val_1);
    }
    @name(".setbyte") action setbyte_0(inout bit<8> dest_2, bit<8> val_2) {
        copy_0(dest_2, val_2);
    }
    @name(".setb1") action setb1_0(bit<9> port) {
        setbyte_0(hdr.data.b1, hdr.data.b2);
        standard_metadata.egress_spec = port;
    }
    @name(".noop") action noop_0() {
    }
    @name(".test1") table test1_0 {
        actions = {
            setb1_0();
            noop_0();
            @defaultonly NoAction();
        }
        key = {
            hdr.data.f1: ternary @name("data.f1") ;
        }
        default_action = NoAction();
    }
    apply {
        test1_0.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
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

