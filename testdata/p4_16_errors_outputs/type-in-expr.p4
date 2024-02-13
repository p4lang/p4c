#include <core.p4>

@foo[bar=4 < H] control p<H>(in H hdrs, out bool flag) {
    apply {
    }
}

