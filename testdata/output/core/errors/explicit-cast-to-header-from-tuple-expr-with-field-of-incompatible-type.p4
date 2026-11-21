extern packet_out {
}

header H {
    bit<32>    a;
    varbit<32> b;
}

control c(packet_out p) {
    apply {
        p.emit((H){ 0, 1 });
    }
}

