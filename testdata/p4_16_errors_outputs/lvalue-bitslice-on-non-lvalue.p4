header hdr {
    bit<32> a;
}

control ingress(hdr h) {
    apply {
        h.a[7:0] = ((bit<32>)h.c)[7:0];
    }
}

