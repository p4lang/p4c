bit f(@optional bit a)
{
  return a;
}

bit g()
{
  return f();
}

control ct();
control c()
{
  apply
  {
    g();
  }
}
package pt(ct c);
pt(c()) main;