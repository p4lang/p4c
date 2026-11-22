// generated from issue3307.p4

typedef bit<1> t;
extern e {
  e();
}
parser MyParser()() {
  e() ee = {
    t f(in int a) {
      return a;
    }
  };
  state start {}
}
