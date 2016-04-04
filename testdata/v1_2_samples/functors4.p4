parser p1()(bit b1)
{
    state start
    {
        bit z1 = b1;
    }
}

parser p()
{
   p1(1w0) p1i;

   state start {
        p1i.apply();
   }
}

parser nothing();
package m(nothing n);

m(p()) main;


