// generated from issue2036-3.p4

struct s {
  bit<8> x;
}
extern void f(in s sarg);
control c()() {
  apply {
    f({ true });
  }
}
