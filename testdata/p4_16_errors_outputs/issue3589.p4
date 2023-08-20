action NoAction() {
}
control eq0() {
    table t {
        actions = {
            NoAction;
        }
    }
    apply {
        bool b = t == t;
    }
}

control ne0() {
    table t {
        actions = {
            NoAction;
        }
    }
    apply {
        bool b = t != t;
    }
}

control c();
package top(c c1, c c2);
top(eq0(), ne0()) main;
