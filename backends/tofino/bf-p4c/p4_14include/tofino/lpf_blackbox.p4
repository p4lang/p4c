/**
 * Copyright (C) 2024 Intel Corporation
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 * 
 * 
 * SPDX-License-Identifier: Apache-2.0
 */


/***************************************************************************/

blackbox_type lpf {

    attribute filter_input {
        /*  Reference to the input field to compute the filter on.
            The maximum input bit width supported is 32 bits.  */
        type: bit<32>;
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


    /*
    Execute the low pass filter for a given cell in the array and writes
    the result to the output parameter.
    If the low pass filter is direct, then 'index' is ignored as the table
    entry determines which cell to reference.

    Callable from:
    - Actions

    Parameters:
    - destination: A field reference to store the low pass filter state.
                   The maximum output bit width is 32 bits.
    - index: Optional. The offset in the low pass filter array to update. Necessary
             unless the low pass filter is declared as direct, in which case it should
             not be present.
    */
    method execute (out bit<32> destination, optional in int index){
        reads {filter_input}
    }
}

/***************************************************************************/
