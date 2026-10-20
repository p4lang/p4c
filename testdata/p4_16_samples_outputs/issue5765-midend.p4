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
    @name("C.output_value") bit<32> output_value_0;
    @name("C.inout_value") bit<32> inout_value_0;
    @name("C.read_value") bit<32> read_value_0;
    @name("C.action_output") bit<32> action_output_0;
    @name("C.action_input_output") bit<32> action_input_output_0;
    @name("C.copy_value") bit<32> copy_value_0;
    @name("C.named_copy_value") bit<32> named_copy_value_0;
    @name("C.reg") Register() reg_0;
    @name("C.before_calls") action before_calls() {
        results.before_out = 32w10;
        results.before_read = 32w30;
    }
    @name("C.observe") action observe() {
        results.after_out = output_value_0;
        results.after_inout = inout_value_0;
        results.after_read = read_value_0;
    }
    @name("C.mutate") action mutate() {
        results.before_action_out = 32w40;
        results.before_action_read = 32w60;
        produce(action_output_0);
        results.action_out = action_output_0;
        modify(action_input_output_0);
        results.action_inout = action_input_output_0;
        reg_0.read(results.action_read, 32w0);
        results.after_member_read = results.action_read;
        copy(copy_value_0, 32w70);
        results.after_copy = copy_value_0;
        copy(source = 32w80, destination = named_copy_value_0);
        results.after_named_copy = named_copy_value_0;
    }
    @hidden action issue5765l70() {
        output_value_0 = 32w10;
        inout_value_0 = 32w20;
        read_value_0 = 32w30;
    }
    @hidden action issue5765l74() {
        produce(output_value_0);
        modify(inout_value_0);
        reg_0.read(read_value_0, 32w0);
    }
    @hidden action issue5765l78() {
        action_output_0 = 32w40;
        action_input_output_0 = 32w50;
        results.action_read = 32w60;
        copy_value_0 = 32w70;
        named_copy_value_0 = 32w80;
    }
    @hidden table tbl_issue5765l70 {
        actions = {
            issue5765l70();
        }
        const default_action = issue5765l70();
    }
    @hidden table tbl_before_calls {
        actions = {
            before_calls();
        }
        const default_action = before_calls();
    }
    @hidden table tbl_issue5765l74 {
        actions = {
            issue5765l74();
        }
        const default_action = issue5765l74();
    }
    @hidden table tbl_observe {
        actions = {
            observe();
        }
        const default_action = observe();
    }
    @hidden table tbl_issue5765l78 {
        actions = {
            issue5765l78();
        }
        const default_action = issue5765l78();
    }
    @hidden table tbl_mutate {
        actions = {
            mutate();
        }
        const default_action = mutate();
    }
    apply {
        tbl_issue5765l70.apply();
        tbl_before_calls.apply();
        tbl_issue5765l74.apply();
        tbl_observe.apply();
        tbl_issue5765l78.apply();
        tbl_mutate.apply();
    }
}

control Pipeline(inout Results results);
package Test(Pipeline pipeline);
Test(C()) main;
