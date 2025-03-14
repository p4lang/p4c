/***************************************************************************/

blackbox_type wred {

    attribute wred_input {
        /*  Reference to the input field to compute the moving average on.
            The maximum input bit width supported is 32 bits.  */
        type: bit<0>;
    }

    attribute direct {
        /* Mutually exclusive with 'static' attribute.
           Must be a match table reference */
        type: table;
        optional;
    }

    attribute static {
        /* Mutually exclusive with 'direct' attribute.
           Must be a table reference */
        type: table;
        optional;
    }

    attribute instance_count {
        /* Mutually exclusive with 'direct' attribute. */
        type: int;
        optional;
    }

    attribute drop_value {
        /* Specifies the lower bound for which the computed moving average should result in a drop. */
        type: int;
        optional;
    }

    attribute no_drop_value {
        /* Specifies the upper bound for which the computed moving average should not result in a drop. */
        type: int;
        optional;
    }

    /*
    Execute the moving average for a given cell in the array and writes
    the result to the output parameter.
    If the wred is direct, then 'index' is ignored as the table
    entry determines which cell to reference.

    Callable from:
    - Actions

    Parameters:
    - destination: A field reference to store the moving average state.
                   The maximum output bit width is 8 bits.
    - index: Optional. The offset in the wred array to update. Necessary
             unless the wred is declared as direct, in which case it should
             not be present.
    */
    method execute (out bit<0> destination, optional in int index){
        reads {wred_input}
    }
}

/***************************************************************************/
