// generated from default-switch.p4

control ctrl()() {
  table t {
    actions = {}
  }
  apply {
    switch ({#}.apply().action_run) {}
  }
}
