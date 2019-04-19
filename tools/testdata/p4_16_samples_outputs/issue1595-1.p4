control c(inout bit<32> b) {
    action a() {
        b = 1;
    }
    table t {
        actions = {
            a;
        }
        default_action = a;
    }
    apply {
        switch (t.apply().action_run) {
            a: {
                b[6:3] = 1;
            }
        }

    }
}

control empty(inout bit<32> b);
package top(empty _e);
top(c()) main;

