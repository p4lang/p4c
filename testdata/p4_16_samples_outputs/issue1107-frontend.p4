#include <core.p4>
#include <v1model.p4>

struct H {
}

struct M {
    bit<32> f1;
    bit<32> f2;
}

parser ParserI(packet_in pk, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        transition accept;
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    @name(".NoAction") action NoAction_0() {
    }
    @name("IngressI.myc.set_eg") action myc_set_eg_0(bit<9> eg) {
        smeta.egress_spec = eg;
    }
    @name("IngressI.myc.myt") table myc_myt {
        key = {
            meta.f1: exact @name("meta.f1") ;
            meta.f2: exact @name("meta.f2") ;
        }
        actions = {
            myc_set_eg_0();
            @defaultonly NoAction_0();
        }
        const entries = {
                        (32w1, 32w0xffffffff) : myc_set_eg_0(9w1);

                        (32w2, 32w0xffffffff) : myc_set_eg_0(9w2);

        }

        default_action = NoAction_0();
    }
    apply {
        myc_myt.apply();
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

