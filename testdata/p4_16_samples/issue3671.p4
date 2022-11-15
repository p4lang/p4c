action NoAction() {}

struct s
{
  bit f0;
  bit f1;
}

extern bit ext();

control c()
{
  bit<2> tt;
  action a(in bit v1, in bit v2) { tt = v1++v2; }
  action a1(in s v) { tt = v.f0++v.f1; }
  table t
  {
    actions = { a1({ f0 = ext(), f1 = ext()}); }
    default_action = a1({ f1 = ext(), f0 = ext()});
  }
  apply {}
}