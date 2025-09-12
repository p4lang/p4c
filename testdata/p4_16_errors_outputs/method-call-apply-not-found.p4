control c(inout bit<8> val)(int a) {
    apply {
    }
}

control ingress(inout bit<8> a) {
    c(0) x;
    apply {
        x.apply();
    }
}

