extern void f(in bit x);
extern void g<T>();

control p()
{
    apply {
        f(); // not enough arguments
        f(1w1, 1w0); // too many arguments
        f<bit>(1w0); // too many type arguments
        g<bit, bit>(); // too many type arguments
    }
}
