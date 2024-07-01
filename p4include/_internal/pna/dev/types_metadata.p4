enum PNA_Source_t {
    FROM_HOST,
    FROM_NET
}

// BEGIN:Metadata_types

struct pna_main_parser_input_metadata_t {
    // common fields initialized for all packets that are input to main
    // parser, regardless of direction.
    bool                     recirculated;
    // If this packet has FROM_NET source, input_port contains
    // the id of the network port on which the packet arrived.
    // If this packet has FROM_HOST source, input_port contains
    // the id of the vport from which the packet came
    PortId_t                 input_port;   // network/host port id
}

// is_host_port(p) returns true if p is a host port, otherwise false.
extern bool is_host_port (in PortId_t p);

// is_net_port(p) returns true if p is a network port, otherwise
// false.
extern bool is_net_port (in PortId_t p);

struct pna_main_input_metadata_t {
    // common fields initialized for all packets that are input to main
    // parser, regardless of direction.
    bool                     recirculated;
    Timestamp_t              timestamp;
    ParserError_t            parser_error;
    ClassOfService_t         class_of_service;
    // See comments for field input_port in struct
    // pna_main_parser_input_metadata_t
    PortId_t                 input_port;
}

// BEGIN:Metadata_main_output
struct pna_main_output_metadata_t {
  // common fields used by the architecture to decide what to do with
  // the packet next, after the main parser, control, and deparser
  // have finished executing one pass, regardless of the direction.
  ClassOfService_t         class_of_service; // 0
}
// END:Metadata_main_output
// END:Metadata_types
