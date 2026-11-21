action a() {
}
action b() {
}
action NoAction() {
}
control c() {
    table t {
        actions = {
            a;
            b;
        }
    }
    apply {
        switch (t.apply().action_run) {
            (true ? a : b): {
            }
        }
    }
}

control C();
package top(C c);
top(c()) main;
