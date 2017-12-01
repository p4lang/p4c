#include <core.p4>

parser parse<H>(out H headers);
package ebpfFilter<H>(parse<H> prs);
struct Headers_t {
}

parser prs(out Headers_t headers) {
    state start {
        transition accept;
    }
}

ebpfFilter<Headers_t>(prs()) main;

