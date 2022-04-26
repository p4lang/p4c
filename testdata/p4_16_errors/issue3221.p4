typedef bit<1> b1;
typedef bit<2> b2;
type b1 t1;
type b2 t2;

t1 func(b2 a)
{
  return (t1)a;
}