control test() {
    action Set_dmac() {
    }
    action drop() {
    }
    table unit() {
        actions = {
            @tableOnly Set_dmac;
            @defaultOnly drop;
        }
    }

    apply {
    }
}

