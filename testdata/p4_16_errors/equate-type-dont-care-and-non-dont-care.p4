// generated from issue1586.p4

extern void random<T>(out T result, in T lo);
control cIngress()() {
  apply {
    random(_, 0);
  }
}
