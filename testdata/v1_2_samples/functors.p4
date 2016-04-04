parser p()(bit b, bit c)
{
   state start {
        bit z = b & c;
   }
}

const bit bv = 1w0;

parser nothing();

package m(nothing n);

m(p(bv, 1w1)) main;

