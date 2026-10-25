// generated from issue1586.p4

extern void random<T>(out T result, in T lo);
control cIngress0() {
  list<match_kind> rand_val;
  apply {
    random(rand_val, 0);
  }
}
