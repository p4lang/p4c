#include <core.p4>

package Ingress<IH>();
package Switch<IH>(Ingress<IH> ingress);

struct H {}

parser ing_parse(out H hdr) {
    state start {
        transition accept;
    }
}

Ingress<H>(ing_parse()) ig1;

Switch<H>(ig1) main;
