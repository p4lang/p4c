match_kind {
    exact
}

typedef bit<48> EthernetAddress;
extern tbl {
    tbl();
}

control c(bit<32> x) {
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
        const entries = {
                        32w0xa000001 : drop();
                        32w0xa000002 : Set_dmac(dmac = (EthernetAddress)48w0x112233445566);
                        32w0xb000003 : Set_dmac(dmac = (EthernetAddress)48w0x112233445577);
                        32w0xb000000 &&& 32w0xff000000 : drop();
        }
        default_action = Set_dmac((EthernetAddress)48w0xaabbccddeeff);
        implementation = tbl();
    }
    apply {
    }
}

