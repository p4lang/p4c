struct s {}

control p()
{
    apply {
        s v;
        bit b;
        
        v.data = 1w0; // no such field
        b.data = 5;   // no such field
    }
}
