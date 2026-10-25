// generated from direct-call1.p4

parser p()() {
  state start {}
}
parser q()() {
  state start {
    p.apply(_);
  }
}
