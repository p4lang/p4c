action NoAction(){}

extern void f<T>(in T a);
control eq0()
{
  table t {
    const size = 100;
    actions = {NoAction;}
  }
  apply {
    f<_>(t);
  }
}