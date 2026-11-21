// generated from issue1586.p4

extern void random<T>(out T result, in T lo);
control cIngress0() {
  bool rand_val;
  apply {
    random(rand_val, 0);
  }
}
