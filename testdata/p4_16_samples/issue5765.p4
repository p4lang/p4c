/*
 * SPDX-FileCopyrightText: 2026 The P4 Language Consortium
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

extern Register {
    Register();
    void read(out bit<32> result, in bit<32> index);
}

extern void produce(out bit<32> result);
extern void modify(inout bit<32> value);
extern void copy(out bit<32> destination, in bit<32> source);

struct Results {
    bit<32> before_out;
    bit<32> before_read;
    bit<32> after_out;
    bit<32> after_inout;
    bit<32> after_read;
    bit<32> before_action_out;
    bit<32> before_action_read;
    bit<32> action_out;
    bit<32> action_inout;
    bit<32> action_read;
    bit<32> after_member_read;
    bit<32> after_copy;
    bit<32> after_named_copy;
}

control C(inout Results results) {
    Register() reg;
    bit<32> output_value;
    bit<32> inout_value;
    bit<32> read_value;
    bit<32> action_output;
    bit<32> action_input_output;
    bit<32> copy_value;
    bit<32> named_copy_value;

    action before_calls() {
        results.before_out = output_value;
        results.before_read = read_value;
    }

    action observe() {
        results.after_out = output_value;
        results.after_inout = inout_value;
        results.after_read = read_value;
    }

    action mutate() {
        results.before_action_out = action_output;
        results.before_action_read = results.action_read;
        produce(action_output);
        results.action_out = action_output;
        modify(action_input_output);
        results.action_inout = action_input_output;
        reg.read(results.action_read, 0);
        results.after_member_read = results.action_read;
        copy(copy_value, copy_value);
        results.after_copy = copy_value;
        copy(source = named_copy_value, destination = named_copy_value);
        results.after_named_copy = named_copy_value;
    }

    apply {
        output_value = 10;
        inout_value = 20;
        read_value = 30;
        before_calls();
        produce(output_value);
        modify(inout_value);
        reg.read(read_value, 0);
        observe();
        action_output = 40;
        action_input_output = 50;
        results.action_read = 60;
        copy_value = 70;
        named_copy_value = 80;
        mutate();
    }
}

control Pipeline(inout Results results);
package Test(Pipeline pipeline);
Test(C()) main;
