// generated from issue3307.p4

typedef bit<1> t;
extern e {
  e();
}
parser MyParser()() {
  e() ee = {
    list<_> f(in t a) {}
  };
  state start {}
}
