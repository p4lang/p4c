#include <core.p4>

enum Either_Tag {
    t,
    None
}

struct Either {
    bit<32>    t;
    Either_Tag tag;
}

control c(out bool o) {
    Either_Tag switch_0_key;
    @hidden action switch_0_default() {
    }
    @hidden action switch_0_case() {
    }
    @hidden table switch_0_table {
        key = {
            switch_0_key: exact;
        }
        actions = {
            switch_0_default();
            switch_0_case();
        }
        const default_action = switch_0_default();
        const entries = {
                        Either_Tag.t : switch_0_case();
        }
    }
    @hidden action safeunion14() {
        o = false;
    }
    @hidden action safeunion12() {
        switch_0_key = Either_Tag.t;
    }
    @hidden table tbl_safeunion12 {
        actions = {
            safeunion12();
        }
        const default_action = safeunion12();
    }
    @hidden table tbl_safeunion14 {
        actions = {
            safeunion14();
        }
        const default_action = safeunion14();
    }
    apply {
        {
            tbl_safeunion12.apply();
            switch (switch_0_table.apply().action_run) {
                switch_0_case: {
                    tbl_safeunion14.apply();
                }
            }
        }
    }
}

control ctrl(out bool o);
package top(ctrl _c);
top(c()) main;

