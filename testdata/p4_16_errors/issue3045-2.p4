#include <core.p4>

extern register<T> {
  register(bit<32> size);
  void write(in bit<32> index, in T value);
}

void f<T>(register<T> r, in T t) {
  r.write(0, t);
}

control c<T>()() {
    register<T>(1) r;

    apply {
        f(r, exact);
    }
}