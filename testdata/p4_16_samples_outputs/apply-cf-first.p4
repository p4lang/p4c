action nop() {
}
control x() {
    table t {
        actions = {
            nop();
        }
        default_action = nop();
    }
    apply {
        if (t.apply().hit) {
            ;
        }
        switch (t.apply().action_run) {
            nop: {
            }
            default: {
            }
        }

    }
}

