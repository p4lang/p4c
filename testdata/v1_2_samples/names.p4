struct S { bit d; }   
const S c = { 1w1 };

control p()
{
    apply {
        S a;
        a.d = c.d;
    }
}
