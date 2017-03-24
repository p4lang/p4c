match_kind {
    exact
}

typedef bit<48> EthernetAddress;
extern tbl {
    tbl();
}

control c(bit<1> x) {
    action Set_dmac(EthernetAddress dmac) {
    }
    action drop() {
    }
    table unit {
        key = {
            x: exact;
        }
        actions = {
            Set_dmac;
            drop;
        }
        default_action = Set_dmac((EthernetAddress)48w0xaabbccddeeff);
        implementation = tbl();
    }
    apply {
    }
}

