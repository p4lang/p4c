#include <core.p4>

control HwControl<H>(inout H hdr);
package Switch<H>(HwControl<H> c);
extern void HwSplFunc<T, R>(in T hdr1, @optional in R hdr2);
header eth_t {
    bit<48> sa;
    bit<48> da;
    bit<16> type;
}

struct parsed_header_t {
    eth_t mac;
}

control match_action_unit(inout parsed_header_t hdr) {
    @hidden action issue2810l27() {
        HwSplFunc<eth_t, _>(hdr.mac);
    }
    @hidden table tbl_issue2810l27 {
        actions = {
            issue2810l27();
        }
        const default_action = issue2810l27();
    }
    apply {
        tbl_issue2810l27.apply();
    }
}

Switch<parsed_header_t>(match_action_unit()) main;
