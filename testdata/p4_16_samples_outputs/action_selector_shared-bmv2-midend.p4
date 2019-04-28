#include <core.p4>
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

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".NoAction") action NoAction_3() {
    }
    @name("IngressI.drop") action drop() {
        mark_to_drop(smeta);
    }
    @name("IngressI.drop") action drop_2() {
        mark_to_drop(smeta);
    }
    @name("IngressI.as") action_selector(HashAlgorithm.identity, 32w1024, 32w10) as_0;
    @name("IngressI.indirect_ws") table indirect_ws_0 {
        key = {
            meta.hash1: selector @name("meta.hash1") ;
        }
        actions = {
            drop();
            NoAction_0();
        }
        const default_action = NoAction_0();
        @name("ap_ws") implementation = as_0;
    }
    @name("IngressI.indirect_ws_1") table indirect_ws_2 {
        key = {
            meta.hash1: selector @name("meta.hash1") ;
        }
        actions = {
            drop_2();
            NoAction_3();
        }
        const default_action = NoAction_3();
        @name("ap_ws") implementation = as_0;
    }
    apply {
        indirect_ws_0.apply();
        indirect_ws_2.apply();
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

