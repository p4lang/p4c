// generated from bool_double_cast.p4

control SetAndFwd()() {
  apply {
    bit<1> a;
    bit<1> b;
    a = (int<1>) b;
  }
}
