// generated from enum.p4

enum X {
  Field,
};
control c()() {
  apply {
    tuple<> v = X.Field;
  }
}
