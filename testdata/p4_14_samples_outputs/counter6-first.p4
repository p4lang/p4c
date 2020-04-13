#include <core.p4>
#define V1MODEL_VERSION 20200408
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
        packet.extract<A_t_0>(hdr.hdrA);
        transition accept;
    }
    @name(".parseB") state parseB {
        packet.extract<B_t_0>(hdr.hdrB);
        transition accept;
    }
    @name(".start") state start {
        packet.extract<type_t>(hdr.packet_type);
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

@name(".cntDum") @min_width(64) counter<bit<12>>(32w4096, CounterType.packets) cntDum;

control processA(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".act") action act(bit<9> port, bit<12> idx) {
        standard_metadata.egress_spec = port;
        cntDum.count(idx);
    }
    @name(".tabA") table tabA {
        actions = {
            act();
            @defaultonly NoAction();
        }
        key = {
            hdr.hdrA.f1: exact @name("hdrA.f1") ;
        }
        default_action = NoAction();
    }
    apply {
        tabA.apply();
    }
}

control processB(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".act") action act(bit<9> port, bit<12> idx) {
        standard_metadata.egress_spec = port;
        cntDum.count(idx);
    }
    @name(".tabB") table tabB {
        actions = {
            act();
            @defaultonly NoAction();
        }
        key = {
            hdr.hdrB.f1: exact @name("hdrB.f1") ;
        }
        default_action = NoAction();
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
        } else if (hdr.hdrB.isValid()) {
            processB_0.apply(hdr, meta, standard_metadata);
        }
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<type_t>(hdr.packet_type);
        packet.emit<B_t_0>(hdr.hdrB);
        packet.emit<A_t_0>(hdr.hdrA);
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

