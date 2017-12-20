#include <core.p4>
#include <v1model.p4>

struct H {
}

struct M {
    bit<32> f1;
    bit<32> f2;
}

typedef bit<32> IPAddr;
const IPAddr MyIP = 32w0xffffffff;
parser ParserI(packet_in pk, out H hdr, inout M meta, inout standard_metadata_t smeta) {
    state start {
        transition accept;
    }
}

action empty() {
}
control myc(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    action set_eg(bit<9> eg) {
        smeta.egress_spec = eg;
    }
    table myt {
        key = {
            meta.f1: exact @name("meta.f1") ;
            meta.f2: exact @name("meta.f2") ;
        }
        actions = {
            set_eg();
            @defaultonly NoAction();
        }
        const entries = {
                        (32w1, 32w0xffffffff) : set_eg(9w1);

                        (32w2, 32w0xffffffff) : set_eg(9w2);

        }

        default_action = NoAction();
    }
    apply {
        myt.apply();
    }
}

control IngressI(inout H hdr, inout M meta, inout standard_metadata_t smeta) {
    @name("myc") myc() myc_inst;
    apply {
        myc_inst.apply(hdr, meta, smeta);
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

