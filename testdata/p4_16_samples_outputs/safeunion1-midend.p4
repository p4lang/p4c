#include <core.p4>

struct S {
    bit<8> f;
}

enum Safe_Tag {
    b,
    c,
    f,
    None
}

struct Safe {
    bit<32>  b;
    bit<16>  c;
    S        f;
    Safe_Tag tag;
}

enum Either_Tag {
    t,
    u,
    None
}

struct Either<T, U> {
    T          t;
    U          u;
    Either_Tag tag;
}

enum Either_0_Tag {
    t,
    u,
    None
}

struct Either_0 {
    bit<32>      t;
    bit<16>      u;
    Either_0_Tag tag;
}

control c(out bool o) {
    @name("c.e") Either_0 e_0;
    Safe_Tag switch_0_key;
    @hidden action switch_0_default() {
    }
    @hidden action switch_0_case() {
    }
    @hidden action switch_0_case_0() {
    }
    @hidden table switch_0_table {
        key = {
            switch_0_key: exact;
        }
        actions = {
            switch_0_default();
            switch_0_case();
            switch_0_case_0();
        }
        const default_action = switch_0_case_0();
        const entries = {
                        Safe_Tag.b : switch_0_case();
        }
    }
    Safe_Tag switch_1_key;
    @hidden action switch_1_default() {
    }
    @hidden action switch_1_case() {
    }
    @hidden action switch_1_case_0() {
    }
    @hidden action switch_1_case_1() {
    }
    @hidden action switch_1_case_2() {
    }
    @hidden table switch_1_table {
        key = {
            switch_1_key: exact;
        }
        actions = {
            switch_1_default();
            switch_1_case();
            switch_1_case_0();
            switch_1_case_1();
            switch_1_case_2();
        }
        const default_action = switch_1_case_2();
        const entries = {
                        Safe_Tag.b : switch_1_case();
                        Safe_Tag.c : switch_1_case_0();
                        Safe_Tag.f : switch_1_case_1();
        }
    }
    Either_0_Tag switch_2_key;
    @hidden action switch_2_default() {
    }
    @hidden action switch_2_case() {
    }
    @hidden action switch_2_case_0() {
    }
    @hidden table switch_2_table {
        key = {
            switch_2_key: exact;
        }
        actions = {
            switch_2_default();
            switch_2_case();
            switch_2_case_0();
        }
        const default_action = switch_2_default();
        const entries = {
                        Either_0_Tag.t : switch_2_case();
                        Either_0_Tag.u : switch_2_case_0();
        }
    }
    @hidden action safeunion1l29() {
        switch_0_key = Safe_Tag.b;
    }
    @hidden action safeunion1l26() {
        switch_1_key = Safe_Tag.b;
    }
    @hidden action safeunion1l20() {
        e_0.tag = Either_0_Tag.None;
    }
    @hidden action safeunion1l39() {
        o = e_0.t == 32w0;
    }
    @hidden action safeunion1l40() {
        o = e_0.u == 16w0;
    }
    @hidden action safeunion1l38() {
        switch_2_key = Either_0_Tag.None;
    }
    @hidden table tbl_safeunion1l20 {
        actions = {
            safeunion1l20();
        }
        const default_action = safeunion1l20();
    }
    @hidden table tbl_safeunion1l26 {
        actions = {
            safeunion1l26();
        }
        const default_action = safeunion1l26();
    }
    @hidden table tbl_safeunion1l29 {
        actions = {
            safeunion1l29();
        }
        const default_action = safeunion1l29();
    }
    @hidden table tbl_safeunion1l38 {
        actions = {
            safeunion1l38();
        }
        const default_action = safeunion1l38();
    }
    @hidden table tbl_safeunion1l39 {
        actions = {
            safeunion1l39();
        }
        const default_action = safeunion1l39();
    }
    @hidden table tbl_safeunion1l40 {
        actions = {
            safeunion1l40();
        }
        const default_action = safeunion1l40();
    }
    apply {
        tbl_safeunion1l20.apply();
        {
            tbl_safeunion1l26.apply();
            switch (switch_1_table.apply().action_run) {
                switch_1_case: {
                }
                switch_1_case_0: {
                    {
                        tbl_safeunion1l29.apply();
                        switch (switch_0_table.apply().action_run) {
                            switch_0_case: {
                            }
                            switch_0_case_0: {
                            }
                        }
                    }
                }
                switch_1_case_1: {
                }
                switch_1_case_2: {
                }
            }
        }
        {
            tbl_safeunion1l38.apply();
            switch (switch_2_table.apply().action_run) {
                switch_2_case: {
                    tbl_safeunion1l39.apply();
                }
                switch_2_case_0: {
                    tbl_safeunion1l40.apply();
                }
            }
        }
    }
}

control ctrl(out bool o);
package top(ctrl _c);
top(c()) main;

