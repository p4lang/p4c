bit h(in bit a)
{
  return a;
}
bit f(in bit a = h(1), in bit b)
{
  return b;
}
bit g(in bit a)
{
  return f(b = a);
}
