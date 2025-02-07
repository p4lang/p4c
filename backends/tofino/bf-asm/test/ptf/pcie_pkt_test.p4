#include <tofino/intrinsic_metadata.p4>
#include <tofino/constants.p4>

parser start {
    return ingress;
}

action set_md(eg_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, eg_port);
}

table port_tbl {
    reads {
        ig_intr_md.ingress_port : exact;
    }
    actions {
        set_md;
    }
    size : 288;
}


control ingress {
    if (0 == ig_intr_md.resubmit_flag) {
        apply(port_tbl);
    }
}

control egress {
}
