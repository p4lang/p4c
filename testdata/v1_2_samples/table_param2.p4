#include "core.p4"

control c(inout bit<32> arg) {
    action a() {}
    table t(inout bit<32> x) {
	key = { x : exact; }
	actions = { a; }
	default_action = a;
    }

    apply {
	if (t.apply(arg).hit)
	    arg = 0;
	else
	    arg = arg + 1;
    }
}

control proto(inout bit<32> arg);
package top(proto p);

top(c()) main;
