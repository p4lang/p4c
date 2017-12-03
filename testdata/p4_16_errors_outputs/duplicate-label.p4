control c(out bit<1> arun) {
    action a() {
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
                arun = 1;
            }
            a: {
                arun = 1;
            }
        }

    }
}

control proto(out bit<1> run);
package top(proto _p);
top(c()) main;

