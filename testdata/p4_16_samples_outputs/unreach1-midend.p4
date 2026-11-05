#include <core.p4>

header h1_t {
    bit<4>[4] a;
    bit<16>   b;
}

header h2_t {
    bit<16> x;
    bit<16> y;
}

struct headers_t {
    h1_t    h1;
    h2_t[4] h2;
}

struct empty_metadata_t {
}

control c(inout headers_t headers, inout empty_metadata_t user_meta) {
    bit<4> switch_0_key;
    bool breakFlag;
    @hidden action switch_0_case() {
    }
    @hidden action switch_0_case_0() {
    }
    @hidden action switch_0_case_1() {
    }
    @hidden table switch_0_table {
        key = {
            switch_0_key: exact;
        }
        actions = {
            switch_0_case();
            switch_0_case_0();
            switch_0_case_1();
        }
        const default_action = switch_0_case_1();
        const entries = {
                        const 4w0 : switch_0_case();
                        const 4w1 : switch_0_case();
                        const 4w2 : switch_0_case();
                        const 4w4 : switch_0_case_0();
        }
    }
    @hidden action unreach1l36() {
        headers.h2[16w0].x = headers.h1.b;
        breakFlag = true;
    }
    @hidden action unreach1l39() {
        headers.h2[16w0].y = headers.h1.b;
    }
    @hidden action unreach1l45() {
        headers.h2[16w0].x = headers.h1.b;
        breakFlag = true;
    }
    @hidden action unreach1l33() {
        breakFlag = false;
        switch_0_key = headers.h1.a[16w0];
    }
    @hidden action unreach1l36_0() {
        headers.h2[16w1].x = headers.h1.b;
        breakFlag = true;
    }
    @hidden action unreach1l39_0() {
        headers.h2[16w1].y = headers.h1.b;
    }
    @hidden action unreach1l45_0() {
        headers.h2[16w1].x = headers.h1.b;
        breakFlag = true;
    }
    @hidden action unreach1l33_0() {
        switch_0_key = headers.h1.a[16w1];
    }
    @hidden action unreach1l36_1() {
        headers.h2[16w2].x = headers.h1.b;
        breakFlag = true;
    }
    @hidden action unreach1l39_1() {
        headers.h2[16w2].y = headers.h1.b;
    }
    @hidden action unreach1l45_1() {
        headers.h2[16w2].x = headers.h1.b;
        breakFlag = true;
    }
    @hidden action unreach1l33_1() {
        switch_0_key = headers.h1.a[16w2];
    }
    @hidden action unreach1l36_2() {
        headers.h2[16w3].x = headers.h1.b;
        breakFlag = true;
    }
    @hidden action unreach1l39_2() {
        headers.h2[16w3].y = headers.h1.b;
    }
    @hidden action unreach1l45_2() {
        headers.h2[16w3].x = headers.h1.b;
        breakFlag = true;
    }
    @hidden action unreach1l33_2() {
        switch_0_key = headers.h1.a[16w3];
    }
    @hidden table tbl_unreach1l33 {
        actions = {
            unreach1l33();
        }
        const default_action = unreach1l33();
    }
    @hidden table tbl_unreach1l36 {
        actions = {
            unreach1l36();
        }
        const default_action = unreach1l36();
    }
    @hidden table tbl_unreach1l39 {
        actions = {
            unreach1l39();
        }
        const default_action = unreach1l39();
    }
    @hidden table tbl_unreach1l45 {
        actions = {
            unreach1l45();
        }
        const default_action = unreach1l45();
    }
    @hidden table tbl_unreach1l33_0 {
        actions = {
            unreach1l33_0();
        }
        const default_action = unreach1l33_0();
    }
    @hidden table tbl_unreach1l36_0 {
        actions = {
            unreach1l36_0();
        }
        const default_action = unreach1l36_0();
    }
    @hidden table tbl_unreach1l39_0 {
        actions = {
            unreach1l39_0();
        }
        const default_action = unreach1l39_0();
    }
    @hidden table tbl_unreach1l45_0 {
        actions = {
            unreach1l45_0();
        }
        const default_action = unreach1l45_0();
    }
    @hidden table tbl_unreach1l33_1 {
        actions = {
            unreach1l33_1();
        }
        const default_action = unreach1l33_1();
    }
    @hidden table tbl_unreach1l36_1 {
        actions = {
            unreach1l36_1();
        }
        const default_action = unreach1l36_1();
    }
    @hidden table tbl_unreach1l39_1 {
        actions = {
            unreach1l39_1();
        }
        const default_action = unreach1l39_1();
    }
    @hidden table tbl_unreach1l45_1 {
        actions = {
            unreach1l45_1();
        }
        const default_action = unreach1l45_1();
    }
    @hidden table tbl_unreach1l33_2 {
        actions = {
            unreach1l33_2();
        }
        const default_action = unreach1l33_2();
    }
    @hidden table tbl_unreach1l36_2 {
        actions = {
            unreach1l36_2();
        }
        const default_action = unreach1l36_2();
    }
    @hidden table tbl_unreach1l39_2 {
        actions = {
            unreach1l39_2();
        }
        const default_action = unreach1l39_2();
    }
    @hidden table tbl_unreach1l45_2 {
        actions = {
            unreach1l45_2();
        }
        const default_action = unreach1l45_2();
    }
    apply {
        tbl_unreach1l33.apply();
        switch (switch_0_table.apply().action_run) {
            switch_0_case: {
                if (headers.h1.b > 16w100) {
                    tbl_unreach1l36.apply();
                } else {
                    tbl_unreach1l39.apply();
                }
            }
            switch_0_case_0: {
                tbl_unreach1l45.apply();
            }
            switch_0_case_1: {
            }
        }
        if (breakFlag) {
            ;
        } else {
            tbl_unreach1l33_0.apply();
            switch (switch_0_table.apply().action_run) {
                switch_0_case: {
                    if (headers.h1.b > 16w100) {
                        tbl_unreach1l36_0.apply();
                    } else if (breakFlag) {
                        ;
                    } else {
                        tbl_unreach1l39_0.apply();
                    }
                }
                switch_0_case_0: {
                    if (breakFlag) {
                        ;
                    } else {
                        tbl_unreach1l45_0.apply();
                    }
                }
                switch_0_case_1: {
                }
            }
            if (breakFlag) {
                ;
            } else {
                tbl_unreach1l33_1.apply();
                switch (switch_0_table.apply().action_run) {
                    switch_0_case: {
                        if (headers.h1.b > 16w100) {
                            tbl_unreach1l36_1.apply();
                        } else if (breakFlag) {
                            ;
                        } else {
                            tbl_unreach1l39_1.apply();
                        }
                    }
                    switch_0_case_0: {
                        if (breakFlag) {
                            ;
                        } else {
                            tbl_unreach1l45_1.apply();
                        }
                    }
                    switch_0_case_1: {
                    }
                }
                if (breakFlag) {
                    ;
                } else {
                    tbl_unreach1l33_2.apply();
                    switch (switch_0_table.apply().action_run) {
                        switch_0_case: {
                            if (headers.h1.b > 16w100) {
                                tbl_unreach1l36_2.apply();
                            } else if (breakFlag) {
                                ;
                            } else {
                                tbl_unreach1l39_2.apply();
                            }
                        }
                        switch_0_case_0: {
                            if (breakFlag) {
                                ;
                            } else {
                                tbl_unreach1l45_2.apply();
                            }
                        }
                        switch_0_case_1: {
                        }
                    }
                }
            }
        }
    }
}

control c_<H, M>(inout H h, inout M m);
package top<H, M>(c_<H, M> c);
top<headers_t, empty_metadata_t>(c()) main;
