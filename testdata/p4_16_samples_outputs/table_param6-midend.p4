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

control c(inout bit<32> arg) {
    bit<32> x_0;
    @name("a") action a() {
    }
    @name("t") table t_0() {
        key = {
            x_0: exact;
        }
        actions = {
            a();
        }
        default_action = a();
    }
    action act() {
        x_0 = arg;
    }
    action act_0() {
        arg = arg + 32w1;
    }
    action act_1() {
        x_0 = arg;
    }
    table tbl_act() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_1() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    apply {
        tbl_act.apply();
        if (t_0.apply().hit) {
            tbl_act_0.apply();
            t_0.apply();
        }
        else 
            tbl_act_1.apply();
    }
}

control proto(inout bit<32> arg);
package top(proto p);
top(c()) main;
