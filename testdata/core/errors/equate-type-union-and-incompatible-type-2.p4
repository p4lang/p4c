// generated from invalid-union.p4

header_union HU {}
parser p(inout tuple<> hu)() {
  state start {
    hu = (HU) {#};
  }
}
