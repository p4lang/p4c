#include <core.p4>

extern widget<T> {
}

extern widget<T> createWidget<T, U>(U a, T b);
parser P();
parser p1()(widget<bit<8>> w) {
    state start {
        transition accept;
    }
}

package sw0(P p);
sw0(p1(createWidget(16w0, 8w0))) main;

