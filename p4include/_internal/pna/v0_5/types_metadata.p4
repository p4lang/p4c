enum PNA_Direction_t {
    NET_TO_HOST,
    HOST_TO_NET
}

// BEGIN:Metadata_types
enum PNA_PacketPath_t {
    // TBD if this type remains, whether it should be an enum or
    // several separate fields representing the same cases in a
    // different form.
    FROM_NET_PORT,
    FROM_NET_LOOPEDBACK,
    FROM_NET_RECIRCULATED,
    FROM_HOST,
    FROM_HOST_LOOPEDBACK,
    FROM_HOST_RECIRCULATED
}

struct pna_pre_input_metadata_t {
    PortId_t                 input_port;
    ParserError_t            parser_error;
    PNA_Direction_t          direction;
    PassNumber_t             pass;
    bool                     loopedback;
}

struct pna_pre_output_metadata_t {
    bool                     decrypt;  // TBD: or use said==0 to mean no decrypt?

    // The following things are stored internally within the decrypt
    // block, in a table indexed by said:

    // + The decryption algorithm, e.g. AES256, etc.
    // + The decryption key
    // + Any read-modify-write state in the data plane used to
    //   implement anti-replay attack detection.

    SecurityAssocId_t        said;
    bit<16>                  decrypt_start_offset;  // in bytes?

    // TBD whether it is important to explicitly pass information to a
    // decryption extern in a way visible to a P4 program about where
    // headers were parsed and found.  An alternative is to assume
    // that the architecture saves the pre parser results somewhere,
    // in a way not visible to the P4 program.
}

struct pna_main_parser_input_metadata_t {
    // common fields initialized for all packets that are input to main
    // parser, regardless of direction.
    PNA_Direction_t          direction;
    PassNumber_t             pass;
    bool                     loopedback;
    // If this packet has direction NET_TO_HOST, input_port contains
    // the id of the network port on which the packet arrived.
    // If this packet has direction HOST_TO_NET, input_port contains
    // the id of the vport from which the packet came
    PortId_t                 input_port;   // network port id
}

struct pna_main_input_metadata_t {
    // common fields initialized for all packets that are input to main
    // parser, regardless of direction.
    PNA_Direction_t          direction;
    PassNumber_t             pass;
    bool                     loopedback;
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
