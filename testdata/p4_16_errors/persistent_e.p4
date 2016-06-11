control p()(bit y)
{
    apply {}
}

control q(in bit z)
{
    p(z) p1;  // argument is not constant
    apply {
        p1.apply();
    }
}

control simple(in bit z);

package m(simple s);

m(q()) main;

