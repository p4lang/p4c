control p()
{
    apply {
        bit a;
        bit b;
        bit c;
        bool d;
        
        a = a ? b : c; // wrong type for a
        d = d ? a : d; // wrong types a <-> d
    }
}
