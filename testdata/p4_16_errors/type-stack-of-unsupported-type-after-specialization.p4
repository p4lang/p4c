// generated from stack-init.p4

header H<T> {
  T t;
}
control c()() {
  apply {
    H<int>[3] s;
  }
}
