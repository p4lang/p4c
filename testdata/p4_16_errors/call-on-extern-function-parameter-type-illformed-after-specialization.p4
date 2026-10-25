// generated from issue2265-1.p4

extern void f<T>(in tuple<T> x);
control c()() {
  apply {
    f<match_kind>({ 32w2 });
  }
}
