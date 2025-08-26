// generated from select-struct.p4

parser p0() {
  state start {
    bit<8> x = 5;
    transition select(x, x, { x, x }) {
      (0, 0, { 0, 0 }, 0): accept;
    }
  }
}
