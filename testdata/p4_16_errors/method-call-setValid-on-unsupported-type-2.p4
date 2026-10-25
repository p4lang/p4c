// generated from issue1386.p4

control compute(inout bool h)() {
  bit<8> n = 0;
  apply {
    if (n > 0) {
      h.setValid();
    }
  }
}
