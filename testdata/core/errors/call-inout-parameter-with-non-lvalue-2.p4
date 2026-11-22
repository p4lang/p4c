// generated from issue3324.p4

void f<t>(inout t a) {}
void g<t>(in t b) {
  f(b);
}
