// generated from default-switch.p4

control ctrl()() {
  table t {
    actions = {}
  }
  apply {
    switch (t.apply().ctrl) {}
  }
}
