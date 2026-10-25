header H {
}

control ingress(inout H hdr) {
    table ipv4_da_lpm {
        key = {
            hdr.dstAddr: lpm;
        }
        actions = {
        }
    }
    apply {
    }
}

