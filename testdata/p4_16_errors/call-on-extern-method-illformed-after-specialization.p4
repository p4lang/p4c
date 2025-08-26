// generated from issue2795.p4

extern packet_out {
  void emit<T>(in T hdr);
}
header H {
  bit<32> a;
  bit<32> b;
}
control c(packet_out p)() {
  apply {
    p.emit<int>((H) { 0, 1 });
  }
}
