// generated from invalid-union.p4

header_union HU {}
parser p(out bool hu)() {
  state start {
    hu = (HU) {#};
  }
}
