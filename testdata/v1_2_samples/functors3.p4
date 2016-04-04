parser p1()(bit b1)
{
    state start
    {
        bit z1 = b1;
    }
}

parser p()(bit b, bit c)
{
   p1(b) p1i;

   state start
   {
        p1i.apply();
        bit z = b & c;
   }
}

const bit bv = 1w0;

parser nothing();
package m(nothing n);

m(p(bv, 1w1)) main;


