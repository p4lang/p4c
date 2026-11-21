extern packet_out {
    abstract void emit<T>(in T hdr);
}

control c(packet_out p) {
    apply {
        p.emit<string>({ 0, 1 });
    }
}

