enum bit<2> e{ t = 1}

e f(in bool c, in bit<2> v0, in bit<2> v1)
{
  return c ? v0 : v1; // { dg-error "" }
}
e f1(in bool c, in bit<2> v0, in e v1)
{
  return c ? v0 : v1; // { dg-error "" }
}
e f2(in bool c, in e v0, in bit<2> v1)
{
  return c ? v0 : v1; // { dg-error "" }
}
