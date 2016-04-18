#include "simple_model.p4"
    
struct Parsed_packet {}

typedef bit<32>  IPv4Address;
    
extern tbl {}

control Pipe(inout Parsed_packet headers,
             in error parseError, // parser error
             in InControl inCtrl, // input port
             out OutControl outCtrl)
{    
    action Drop_action(out PortId_t port)
    {
        port = DROP_PORT; 
    }

    action drop() {}
    
    table IPv4_match(in IPv4Address a)
    {
        actions = {
            drop;
        }
        key = { inCtrl.inputPort : exact; }
        implementation = tbl();
        default_action = drop;
    }
        
    apply {
        if (parseError != NoError)
        {
            // invoke Drop_action directly
            Drop_action(outCtrl.outputPort);
            return;
        }
        
        IPv4Address nextHop; // temporary variable
        if (!IPv4_match.apply(nextHop).hit)
            return;
    }
}
