// generated from generic-struct-tuple.p4

struct S<T> {
  tuple<T, T> t;
}
const S<void> x = { t = { 0, 0 } };
