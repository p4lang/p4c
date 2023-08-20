bit f(in bit t0)
{
    return t0;
}

bit<2> f(in bit<2> t)
{
    bit b0;
    bit b1;
    b0 = (bit)t;
    b1 = (bit)(t>>1);
    b0 = f(b0);
    b1 = f(b1);
    bit<2> b = (bit<2>)b0;
    b = b | (((bit<2>)b1)<<1);
    return b;
}