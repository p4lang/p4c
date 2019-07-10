typedef tuple<> emptyTuple;
control c(out bool b) {
    emptyTuple t_0;
    apply {
        t_0 = {  };
        if (t_0 == {  }) {
            b = true;
        } else {
            b = false;
        }
    }
}

control e(out bool b);
package top(e _e);
top(c()) main;

