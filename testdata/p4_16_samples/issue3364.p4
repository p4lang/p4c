typedef bit<8> i8;
enum i8 e
{
  value0 = 0
}
void f(in bit<8> yy) {}
void g(in e yy)
{
  f(yy);
}
