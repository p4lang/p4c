#include <core.p4>
#include <v1model.p4>

@name("A_t") header A_t_0 {
    bit<32> f1;
    bit<32> f2;
    bit<16> h1;
    bit<8>  b1;
    bit<8>  b2;
}

@name("B_t") header B_t_0 {
    bit<8>  b1;
    bit<8>  b2;
    bit<16> h1;
    bit<32> f1;
    bit<32> f2;
}

header type_t {
    bit<16> type;
}

struct metadata {
}

struct headers {
    @name(".hdrA") 
    A_t_0  hdrA;
    @name(".hdrB") 
    B_t_0  hdrB;
    @name(".packet_type") 
    type_t packet_type;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parseA") state parseA {
        packet.extract(hdr.hdrA);
        transition accept;
    }
    @name(".parseB") state parseB {
        packet.extract(hdr.hdrB);
        transition accept;
    }
    @name(".start") state start {
        packet.extract(hdr.packet_type);
        transition select(hdr.packet_type.type) {
            16w0xa &&& 16w0xf: parseA;
            16w0xb &&& 16w0xf: parseB;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

@name(".cntDum") @min_width(64) counter(32w4096, CounterType.packets) cntDum;

control processA(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".act") action act(bit<9> port, bit<32> idx) {
        standard_metadata.egress_spec = port;
        cntDum.count((bit<32>)idx);
    }
    @name(".tabA") table tabA {
        actions = {
            act;
        }
        key = {
            hdr.hdrA.f1: exact;
        }
    }
    apply {
        tabA.apply();
    }
}

control processB(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".act") action act(bit<9> port, bit<32> idx) {
        standard_metadata.egress_spec = port;
        cntDum.count((bit<32>)idx);
    }
    @name(".tabB") table tabB {
        actions = {
            act;
        }
        key = {
            hdr.hdrB.f1: exact;
        }
    }
    apply {
        tabB.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".processA") processA() processA_0;
    @name(".processB") processB() processB_0;
    apply {
        if (hdr.hdrA.isValid()) {
            processA_0.apply(hdr, meta, standard_metadata);
        } else {
            if (hdr.hdrB.isValid()) {
                processB_0.apply(hdr, meta, standard_metadata);
            }
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.packet_type);
        packet.emit(hdr.hdrB);
        packet.emit(hdr.hdrA);
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

