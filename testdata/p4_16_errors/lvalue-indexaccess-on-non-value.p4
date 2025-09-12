// generated from gauntlet_parser_test.p4

header Hdr {
  bit<8> x;
}
struct Headers {
  Hdr[2] h2;
}
parser P(in Headers h)() {
  state start {
    h.h2[1] = 0;
  }
}
