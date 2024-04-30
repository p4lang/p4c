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
    @hidden action switch_0_case() {
    }
    @hidden action switch_0_case_0() {
    }
    @hidden table switch_0_table {
        key = {
            hdr.payload.x: exact;
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
    @hidden action switch_1_case() {
    }
    @hidden action switch_1_case_0() {
    }
    @hidden table switch_1_table {
        key = {
            hdr.payload.x: exact;
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
    @hidden action switch_2_case() {
    }
    @hidden action switch_2_case_0() {
    }
    @hidden action switch_2_case_1() {
    }
    @hidden table switch_2_table {
        key = {
            hdr.payload.x: exact;
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
    apply {
        switch (switch_2_table.apply().action_run) {
            switch_2_case: {
                switch (switch_0_table.apply().action_run) {
                    switch_0_case: {
                    }
                    switch_0_case_0: {
                    }
                }
            }
            switch_2_case_0: {
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
