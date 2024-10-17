/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

/***************************************************************************/

blackbox_type stateful_alu {

    attribute reg {
        /*  Reference to the register table description. */
        type: register;
    }

    attribute selector_binding {
        /*  Bind this stateful ALU to the selector used by the specified match table. */
        type: table;
        optional;
    }

    attribute initial_register_lo_value {
        /*  The initial value to use for the stateful memory cell.  Used in dual-width mode as the
            low half initial value.  In single-width mode, this is the initial value of the entire memory cell.
        */
        type: int;
        optional;
    }

    attribute initial_register_hi_value {
        /*  The initial value to use for the stateful memory cell.  Only relevant in dual-width mode to
            specify the high half initial value.
        */
        type: int;
        optional;
    }

    attribute condition_hi {
        /* Condition associated with cmp hi unit.
           An expression that can be transformed into the form:
             memory +/- phv - constant operation 0
         */
        type: expression;
        expression_local_variables { int register_lo, int register_hi}
        optional;
    }

    attribute condition_lo {
        /* Condition associated with cmp lo unit.
           An expression that can be transformed into the form:
             memory +/- phv - constant operation 0
         */
        type: expression;
        expression_local_variables { int register_lo, int register_hi}
        optional;
    }


    attribute update_lo_1_predicate {
        /* Condition expression associated with running ALU 1 lo. */
        type: expression;
        expression_local_variables { bool condition_lo, bool condition_hi}
        optional;
    }

    attribute update_lo_1_value {
        /* Expression computed in ALU 1 lo.
           In single-bit mode, this ALU can only perform single_bit instructions, which perform the following:

           +-------------------+---------------------------------------------------------------------------------+
           |  Operation Name   |   Description                                                                   |
           +-------------------+---------------------------------------------------------------------------------+
           |    set_bit        |  Sets the specified bit to 1, outputs the previous bit value.                   |
           +-------------------+---------------------------------------------------------------------------------+
           |    set_bitc       |  Sets the specified bit to 1, outputs the complement of the previous bit value. |
           +-------------------+---------------------------------------------------------------------------------+
           |    clr_bit        |  Sets the specified bit to 0, outputs the previous bit value.                   |
           +-------------------+---------------------------------------------------------------------------------+
           |    clr_bitc       |  Sets the specified bit to 0, outputs the complement of the previous bit value. |
           +-------------------+---------------------------------------------------------------------------------+
           |    read_bit       |  Does not modify specified bit, outputs current bit value.                      |
           +-------------------+---------------------------------------------------------------------------------+
           |    read_bitc      |  Does not modify specified bit, outputs complement of the current bit value.    |
           +-------------------+---------------------------------------------------------------------------------+

         */
        type: expression;
        expression_local_variables {int register_lo, int register_hi,
                                    set_bit, set_bitc, clr_bit, clr_bitc, read_bit, read_bitc}
        optional;
    }

    attribute update_lo_2_predicate {
        /* Condition expression associated with running ALU 2 lo. */
        type: expression;
        expression_local_variables {bool condition_lo, bool condition_hi}
        optional;
    }

    attribute update_lo_2_value {
        /* Expression computed in ALU 2 lo. */
        type: expression;
        expression_local_variables {int register_lo, int register_hi, int math_unit}
        optional;
    }

    attribute update_hi_1_predicate {
        /* Condition expression associated with running ALU 1 hi. */
        type: expression;
        expression_local_variables {bool condition_lo, bool condition_hi}
        optional;
    }

    attribute update_hi_1_value {
        /* Expression computed in ALU 1 hi. */
        type: expression;
        expression_local_variables {int register_lo, int register_hi}
        optional;
    }

    attribute update_hi_2_predicate {
        /* Condition expression associated with running ALU 2 hi. */
        type: expression;
        expression_local_variables {bool condition_lo, bool condition_hi}
        optional;
    }

    attribute update_hi_2_value {
        /* Expression computed in ALU 2 hi. */
        type: expression;
        expression_local_variables {int register_lo, int register_hi}
        optional;
    }

    attribute output_predicate {
        /* Condition expression associated with outputting result to action data bus.
           Allowed references are 'condition_lo' and 'condition_hi'.
           Allowed operations are 'and', 'or', and 'not'.
         */
        type: expression;
        expression_local_variables {bool condition_lo, bool condition_hi}
        optional;
    }

    attribute output_value {
        /* Output result expression.
           Allowed references are 'alu_lo', 'alu_hi', 'register_lo', 'register_hi',
           'predicate', and 'combined_predicate'.
           In the case of 'register_lo' and 'register_hi', these are the values
           fetched from memory on this access, not the value to be written back
           computed by the ALU(s).
         */
        type: expression;
        expression_local_variables {int alu_lo, int alu_hi, int register_lo, int register_hi, int predicate, int combined_predicate}
        optional;
    }

    attribute output_dst {
        /* Optional field to write the stateful result to. */
        type: int;
        optional;
    }

    attribute math_unit_input {
        /* Specification of the math unit's input.
           This attribute must be defined if a math_unit is referenced in 'update_lo_2_value'.
         */
        type: expression;
        expression_local_variables {int register_lo, int register_hi}
        optional;
    }
    attribute math_unit_output_scale {
        /* Specification of the math unit's output scale.
           This is a 6-bit signed number added to the exponent, which shifts the output result.
           The default value is 0.
         */
        type: int;
        optional;
    }
    attribute math_unit_exponent_shift {
        /* Specification of the math unit's exponent shift.
           Supported shift values are -1, 0, and 1.  The default value is 0.
           This value controls the shifting of the input data exponent.
           With an exponent shift of -1, the math unit normalization is 2-bit resolution.
           Otherwise, the math unit normalization is 1-bit resolution.
         */
        type: int;
        optional;
    }
    attribute math_unit_exponent_invert {
        /* Specifies whether the math unit should invert the computed exponent.
           By default, the exponent is not inverted. Allowed values are true and false.
           Inverting allows reciprocal exponents, e.g. 1/x, 1/x**2, or 1/sqrt(x)
         */
        type: bit;
        optional;
    }

    attribute math_unit_lookup_table {
        /* Specifies the math unit's lookup table values.
           There are 16 8-bit values that can be looked up.
           This attribute must be defined if a math_unit is referenced in 'update_lo_2_value'.
           Input is specified with the most significant byte appearing first in the string.
           For example:
           0x0f 0x0e 0x0d 0x0c 0x0b 0x0a 9 8 7 6 5 4 3 2 1 0;
         */
        type: string;
        optional;
    }

    attribute reduction_or_group {
        /* Specifies that the output value to be written belongs to a group of stateful ALU
           outputs that are all OR'd together.  Using a reduction OR group breaks what
           would normally be considered a dependency.
           A typical use case is to perform a membership check in, e.g., a Bloom filter.
           For example, n unique hash functions may be used to check for membership in
           n unique registers.  If any of the hashed locations are active,
           the member is active.
         */
        type: string;
        optional;
    }

    attribute stateful_logging_mode {
        /*  Specify that stateful logging should be performed.
            Stateful logging writes a result to consecutive addresses in a register based on the mode
            of logging being performed.
            Allowed values are:
               table_hit - performing logging if the match table hits and is predicated on.
               table_miss - performing logging if the match table misses and is predicated on.
               gateway_inhibit - performing logging if the gateway table inhibits a match table and is predicated on.
               address - any time the table is predicated on.
         */
        type: string;
        optional;
    }

    /*
    Executes this stateful alu instance.

    If output_dst is defined, this writes the output_value to a destination field.

        output_dst = output_value

    Callable from:
    - Actions

    Parameters:
    - index: The offset into the stateful register to access.
      May be a constant or an address provided by the run time.
    */
    method execute_stateful_alu(optional in bit<32> index){
        reads {condition_hi, condition_lo,
               update_lo_1_predicate, update_lo_1_value,
               update_lo_2_predicate, update_lo_2_value,
               update_hi_1_predicate, update_hi_1_value,
               update_hi_2_predicate, update_hi_2_value,
               math_unit_input}
        writes {output_dst}
    }

    method execute_stateful_alu_from_hash(in field_list_calculation hash_field_list){
        reads {condition_hi, condition_lo,
               update_lo_1_predicate, update_lo_1_value,
               update_lo_2_predicate, update_lo_2_value,
               update_hi_1_predicate, update_hi_1_value,
               update_hi_2_predicate, update_hi_2_value,
               math_unit_input}
        writes {output_dst}
    }

    method execute_stateful_log(){
        reads {condition_hi, condition_lo,
               update_lo_1_predicate, update_lo_1_value,
               update_lo_2_predicate, update_lo_2_value,
               update_hi_1_predicate, update_hi_1_value,
               update_hi_2_predicate, update_hi_2_value,
               math_unit_input}
    }
}

/***************************************************************************/
