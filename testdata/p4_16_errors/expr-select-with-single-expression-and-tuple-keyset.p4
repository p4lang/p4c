// generated from issue356-bmv2.p4

extern packet_in {
  T lookahead<T>();
}
parser parserI(packet_in pkt) {
  state start {
    transition select(pkt.lookahead<bit<112>>()[111:96]) {
      (16w4096 &&& 16w4096, 16w4096 &&& 16w4096): accept;
    }
  }
}
