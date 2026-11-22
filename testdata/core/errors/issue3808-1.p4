action NoAction(){}

extern f<T>
{
  f();
  void g(in T a);
}
control eq0(f<_> tt)
{
  table t {
    const size = 100;
    actions = {NoAction;}
  }
  apply {
    tt.g(t);
  }
}