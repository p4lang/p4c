// Function calls with template parameters are parsed with the wrong priority.
// This is a bug in the bison grammar which is hard to fix.
// The workaround is to use parentheses.

extern T f<T>(T x);

action a()
{
    bit<32> x;
    x = (bit<32>)f<bit<6>>(6w5);
}
