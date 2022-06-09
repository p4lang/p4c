/* -*- P4_16 -*- */
#include <core.p4>
#include <v1model.p4>

struct H {};
struct M {
    bit<8> m0;
};
parser P(packet_in p, out H h, inout M m, inout standard_metadata_t s) {
    state start {
	m.m0 = 0;
	transition accept;
    }
}
control I(inout H h, inout M m, inout standard_metadata_t s) {
    action a0() {
	switch (m.m0) {
	    8w0x0: {}
	}
    }
    action a1(in bit<8> m0) {
	switch (m0) {
	    8w0x0: {}
	}
    }
    apply {
	#define CASE 0
	#if (CASE == 0)
	a0();
	#endif
	#if (CASE == 1)
	a1(m.m0);
	#endif
    }
}

control E(inout H h, inout M m, inout standard_metadata_t s) {apply {}}
control D(packet_out p, in H h) {apply {}}
control V(inout H h, inout M m) {apply {}}
control C(inout H h, inout M m) {apply {}}

V1Switch(P(), V(), I(), E(), C(), D()) main;
