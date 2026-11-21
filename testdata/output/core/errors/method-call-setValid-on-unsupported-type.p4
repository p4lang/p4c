struct hdr {
}

control compute(inout hdr h) {
    bit<8> n = 0;
    apply {
        if (n > 0) {
            h.setValid();
        }
    }
}

