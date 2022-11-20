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
    actions = { a1((s){ ext(),  ext()}); }
    default_action = a1((s){ ext(), ext()});
  }
  apply {}
}