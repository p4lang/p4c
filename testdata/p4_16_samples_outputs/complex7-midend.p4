struct Version {
    bit<8> major;
    bit<8> minor;
}

error {
    NoError,
    PacketTooShort,
    NoMatch,
    EmptyStack,
    FullStack,
    OverwritingHeader,
    HeaderTooShort
}

extern packet_in {
    void extract<T>(out T hdr);
    void extract<T>(out T variableSizeHeader, in bit<32> variableFieldSizeInBits);
    T lookahead<T>();
    void advance(in bit<32> sizeInBits);
    bit<32> length();
}

extern packet_out {
    void emit<T>(in T hdr);
}

match_kind {
    exact,
    ternary,
    lpm
}

extern bit<32> f(in bit<32> x);
control c(inout bit<32> r) {
    bit<32> r_0;
    bit<32> tmp;
    bit<32> tmp_0;
    @name("a") action a() {
    }
    @name("b") action b() {
    }
    @name("t") table t_0() {
        key = {
            r_0: ternary;
        }
        actions = {
            a();
            b();
        }
        default_action = a();
    }
    action act() {
        r = 32w1;
    }
    action act_0() {
        r = 32w3;
    }
    action act_1() {
        tmp = f(32w2);
    }
    action act_2() {
        tmp_0 = f(32w2);
        r_0 = tmp_0;
    }
    table tbl_act() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    table tbl_act_0() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_1() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_2() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        switch (t_0.apply().action_run) {
            a: {
                tbl_act_0.apply();
                if (tmp < 32w2) 
                    tbl_act_1.apply();
                else 
                    tbl_act_2.apply();
            }
        }

    }
}

control simple(inout bit<32> r);
package top(simple e);
top(c()) main;
