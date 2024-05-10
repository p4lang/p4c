extern void foo();
extern void bar();
extern bit<8> baz();
control c() {
    @name("c.tmp") bit<8> tmp;
    @name(".a") action a_0() {
    }
    @name(".b") action b_0() {
    }
    @name(".NoAction") action NoAction_1() {
    }
    @name("c.t") table t_0 {
        actions = {
            a_0();
            b_0();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        tmp = baz();
        switch (tmp) {
            8w1: {
                foo();
            }
            8w4: {
                bar();
            }
        }
        t_0.apply();
    }
}

control C();
package top(C c);
top(c()) main;
