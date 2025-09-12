// generated from issue2265-1.p4

extern void f<T>(in list<bool> x);
control c()() {
  apply {
    f<bit<32>>({ 32w2 });
  }
}
