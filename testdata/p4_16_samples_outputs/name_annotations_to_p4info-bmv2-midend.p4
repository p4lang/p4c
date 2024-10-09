#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

struct H {
}

struct M {
    bit<32> hash1;
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        transition accept;
    }
}

@name("IngressI.altIngressName") control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("IngressI.blue") action foo(@name("orange") bit<9> port) {
        smeta.egress_spec = port;
    }
    @name("IngressI.red") table foo_table_0 {
        key = {
            smeta.ingress_port: exact @name("green");
        }
        actions = {
            @name("yellow") foo();
            @name("grey") NoAction_1();
        }
        const default_action = NoAction_1();
    }
    apply {
        foo_table_0.apply();
    }
}

control EgressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    apply {
    }
}

control DeparserI(packet_out pk, in H hdr) {
    apply {
    }
}

control VerifyChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

control ComputeChecksumI(inout H hdr, inout M meta) {
    apply {
    }
}

V1Switch<H, M>(ParserI(), VerifyChecksumI(), IngressI(), EgressI(), ComputeChecksumI(), DeparserI()) main;
