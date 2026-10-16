struct S { }
struct R {}
struct T { R r; bit<8> b; }

control Impl() {
  apply {
    T t = { r = ..., ... };
    S s = { b = ... };

    if (t == {{}, 1} && s == {}) {
      t.b = 0;
    }
  }
}

control C();
package P(C c);
P(Impl()) main;