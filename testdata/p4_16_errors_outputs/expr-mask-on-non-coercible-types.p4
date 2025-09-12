extern packet_in {
    T lookahead<T>();
}

parser parserI(packet_in pkt) {
    state start {
        transition select((pkt.lookahead<bit<112>>())[111:96]) {
            (16w4096 &&& 16s4096): accept;
        }
    }
}

