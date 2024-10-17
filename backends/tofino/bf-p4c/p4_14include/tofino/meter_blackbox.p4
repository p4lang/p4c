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

blackbox_type meter {

    attribute type {
        /* Must be either:
            bytes
            packets */
        type: string;
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

    attribute green_value {
        /* An 8-bit value that can be output if the packet is to be marked as green.
           The default value is 0. */
        type: int;
        optional;
    }
    attribute yellow_value {
        /* An 8-bit value that can be output if the packet is to be marked as yellow.
           The default value is 1. */
        type: int;
        optional;
    }
    attribute red_value {
        /* An 8-bit value that can be output if the packet is to be marked as red.
           The default value is 3. */
        type: int;
        optional;
    }

    attribute meter_profile {
        /* A 4-bit designation of what meter rate profile to use.  The rates supported by
           each profile are summarized in the table below.

           +-----------+-----------------------+---------------------------+-----------------------+
           |  Profile  |  Min Rate (Bytes/sec) |  Max Rate (Bytes/second)  |  Max Rate (Pkts/sec)  |
           +-----------+-----------------------+---------------------------+-----------------------+
           |     0     |         9.46          |        648970000000       |      1270000000       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |     1     |         4.73          |        324485000000       |       635000000       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |     2     |         2.37          |        162242500000       |       317500000       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |     3     |         1.18          |         81121250000       |       158750000       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |     4     |         0.59          |         40560625000       |        79375000       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |     5     |         0.30          |         20280312500       |        39687500       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |     6     |         0.15          |         10140156250       |        19843750       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |     7     |        0.074          |          5070078125       |         9921875       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |     8     |        0.037          |          2535039063       |         4960938       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |     9     |        0.018          |          1267519531       |         2480468       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |    10     |       0.0092          |           633759766       |         1240234       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |    11     |       0.0046          |           316879883       |          620117       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |    12     |       0.0023          |           158439941       |          310059       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |    13     |       0.0012          |            79219970       |          155029       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |    14     |      0.00058          |            39609985       |           77515       |
           +-----------+-----------------------+---------------------------+-----------------------+
           |    15     |      0.00029          |            19804992       |           38757       |
           +-----------+-----------------------+---------------------------+-----------------------+

           The default profile value is 0.
         */
        type: int;
        optional;
    }

    /*
    Execute the metering operation for a given cell in the array. If
    the  meter is direct, then 'index' is ignored as the table
    entry determines which cell to reference. The length of the packet
    is implicitly passed to the meter. The state of meter is updated
    and the meter returns information (a 'color') which is stored in
    'destination'. If the parent header of 'destination' is not valid,
    the meter  state is updated, but resulting output is discarded.

    Callable from:
    - Actions

    Parameters:
    - destination: A field reference to store the low pass filter state.
                   The maximum output bit width is 32 bits.
    - index: Optional. The offset in the low pass filter array to update. Necessary
             unless the low pass filter is declared as direct, in which case it should
             not be present.
    */
    method execute (out bit<32> destination, optional in int index){}

    /* Same as execute, but destination field is OR'd with meter result. */
    method execute_with_or (out bit<32> destination, optional in int index){}

    /*  Same as execute, but the precolor attribute specifies the minimum color the packet
        may be tagged with.
        Pre-color encoding (which is not programmable):
        0 = green
        1 = yellow
        2 = yellow
        3 = red
     */
    method execute_with_pre_color (out bit<32> destination, in bit<32> precolor, optional in int index){}

    /* Same as execute_with_pre_color, but destination field is OR'd with meter result.  */
    method execute_with_pre_color_with_or(out bit<32> destination, in bit<32> precolor, optional in int index){}
}

/***************************************************************************/
