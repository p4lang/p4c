void h(in bit t)
{

}

control g(in bit t)(@optional bit tt)
{
  apply { h(tt);}
}

control ct();
control c()
{
  g() y;
  apply
  {
    y.apply(1);
  }
}
package pt(ct c);
pt(c()) main;