from runtime import Runtime
from runtime.ttypes import *

from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

import struct

def bytes_to_string(byte_array):
    form = 'B' * len(byte_array)
    return struct.pack(form, *byte_array)

def table_error_name(x):
    return TableOperationErrorCode._VALUES_TO_NAMES[x]

def main():
    # Make socket
    transport = TSocket.TSocket('localhost', 9090)
    
    # Buffering is critical. Raw sockets are very slow
    transport = TTransport.TBufferedTransport(transport)
    
    # Wrap in a protocol
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    
    # Create a client to use the protocol encoder
    client = Runtime.Client(protocol)

    # Connect!
    transport.open()

    client.bm_set_default_action("forward", "_drop", [])

    try:
        entry_handle = client.bm_table_add_exact_match_entry(
            "forward", "set_egress_port",
            bytes_to_string([0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff]),
            [bytes_to_string([2])]
        )

        print "entry added", entry_handle
    except InvalidTableOperation, ito:
        print "Error:", table_error_name(ito.what)

if __name__ == "__main__":
    main()
