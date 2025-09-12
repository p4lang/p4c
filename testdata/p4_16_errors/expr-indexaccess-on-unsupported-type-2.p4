// generated from stack-init.p4

control c(out bit<32> r)() {
  apply {
    list<string> s;
    r = s[2];
  }
}
