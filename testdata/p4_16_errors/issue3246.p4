bit<32> func()
{
  return (1w1).minSizeInBits<int, int, int, int, int>();
}

bit<32> func()
{
  return (1w1).minSizeInBits<int, int, int, int, int>(_,_,_);
}

bit<32> func1()
{
  return (1w1).minSizeInBits<int, int, int, int, int>(a = 1, b = 1, c = 1);
}
