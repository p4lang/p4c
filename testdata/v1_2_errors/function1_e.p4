extern void f(out bit x);

control p()
{
    apply {
        f(1w1 & 1w0); // non lvalue passed to out parameter
    }
}
