// generated from default-switch.p4

control ctrl()() {
  action b() {}
  table t {
    actions = {}
  }
  apply {
    switch (t.apply().action_run) {
      .b:
      default:
    }
  }
}
