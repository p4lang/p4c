control p()
{
    action a(bit x0, out bit y0)
    {
        bit x = x0;
        y0 = x0 & x;
    }
    
    action b(bit x, out bit y)
    {
        bit z;
        a(x, z);
        a(z & z, y);
    }

    apply {
        bit x;
        bit y;
        b(x, y);
    }
}

package m(p pipe);

m(p()) main;
