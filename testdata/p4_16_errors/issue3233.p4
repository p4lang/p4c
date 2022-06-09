struct s
{
  bit t;
  bit t1;
}

s func(bit t, bit t1)
{
  return (s)(s)(s){t, t1};
}

s func1(s t1)
{
  return (s)(s)(s)t1;
}
