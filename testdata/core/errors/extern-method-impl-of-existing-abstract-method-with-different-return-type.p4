// generated from issue3307.p4

typedef bit<1> t;
extern e {
  e();
  abstract int f(in t a);
}
parser MyParser0() {
  e() ee = {
    t f(in t a) {
      return a;
    }
  };
  state start {}
}
