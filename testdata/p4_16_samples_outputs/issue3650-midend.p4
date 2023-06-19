#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header payload_t {
    bit<8> x;
}

struct header_t {
    payload_t payload;
}

struct metadata {
}

parser MyParser(packet_in packet, out header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<payload_t>(hdr.payload);
        transition accept;
    }
}

control MyIngress(inout header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    bit<8> switch_0_key;
    @hidden action switch_0_case() {
    }
    @hidden action switch_0_case_0() {
    }
    @hidden table switch_0_table {
        key = {
            switch_0_key: exact;
        }
        actions = {
            switch_0_case();
            switch_0_case_0();
        }
        const default_action = switch_0_case_0();
        const entries = {
                        const 8w1 : switch_0_case();
        }
    }
    bit<8> switch_1_key;
    @hidden action switch_1_case() {
    }
    @hidden action switch_1_case_0() {
    }
    @hidden table switch_1_table {
        key = {
            switch_1_key: exact;
        }
        actions = {
            switch_1_case();
            switch_1_case_0();
        }
        const default_action = switch_1_case_0();
        const entries = {
                        const 8w1 : switch_1_case();
        }
    }
    bit<8> switch_2_key;
    @hidden action switch_2_case() {
    }
    @hidden action switch_2_case_0() {
    }
    @hidden action switch_2_case_1() {
    }
    @hidden table switch_2_table {
        key = {
            switch_2_key: exact;
        }
        actions = {
            switch_2_case();
            switch_2_case_0();
            switch_2_case_1();
        }
        const default_action = switch_2_case_1();
        const entries = {
                        const 8w1 : switch_2_case();
                        const 8w0 : switch_2_case_0();
        }
    }
    @hidden action issue3650l24() {
        switch_0_key = hdr.payload.x;
    }
    @hidden action issue3650l24_0() {
        switch_1_key = hdr.payload.x;
    }
    @hidden action issue3650l35() {
        switch_2_key = hdr.payload.x;
    }
    @hidden table tbl_issue3650l35 {
        actions = {
            issue3650l35();
        }
        const default_action = issue3650l35();
    }
    @hidden table tbl_issue3650l24 {
        actions = {
            issue3650l24();
        }
        const default_action = issue3650l24();
    }
    @hidden table tbl_issue3650l24_0 {
        actions = {
            issue3650l24_0();
        }
        const default_action = issue3650l24_0();
    }
    apply {
        tbl_issue3650l35.apply();
        switch (switch_2_table.apply().action_run) {
            switch_2_case: {
                tbl_issue3650l24.apply();
                switch (switch_0_table.apply().action_run) {
                    switch_0_case: {
                    }
                    switch_0_case_0: {
                    }
                }
            }
            switch_2_case_0: {
                tbl_issue3650l24_0.apply();
                switch (switch_1_table.apply().action_run) {
                    switch_1_case: {
                    }
                    switch_1_case_0: {
                    }
                }
            }
            switch_2_case_1: {
            }
        }
    }
}

control MyVerifyChecksum(inout header_t hdr, inout metadata meta) {
    apply {
    }
}

control MyEgress(inout header_t hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control MyDeparser(packet_out packet, in header_t hdr) {
    apply {
    }
}

control MyComputeChecksum(inout header_t hdr, inout metadata meta) {
    apply {
    }
}

V1Switch<header_t, metadata>(MyParser(), MyVerifyChecksum(), MyIngress(), MyEgress(), MyComputeChecksum(), MyDeparser()) main;
