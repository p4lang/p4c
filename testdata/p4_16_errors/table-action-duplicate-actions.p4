// generated from issue4661_non_pure_extern_function_const_args.p4

action a() {}
control c()() {
  table t {
    actions = {
      a;
      a;
    }
  }
  apply {}
}
