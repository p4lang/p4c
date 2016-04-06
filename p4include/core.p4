/* This is the P4 v1.2 core library, which declares some built-in P4 constructs using P4 */

#ifndef _CORE_P4_
#define _CORE_P4_

struct Version {
    bit<8> major;
    bit<8> minor;
}

const Version P4_VERSION = { 8w1, 8w2 };

error {
    NoError,          // no error
    PacketTooShort,   // not enough bits in packet for extract
    NoMatch,          // match expression has no matches
    EmptyStack,       // reference to .last in an empty header stack
    FullStack,        // reference to .next in a full header stack
    OverwritingHeader // one header is extracted twice
}

extern packet_in {
    void extract<T>(out T hdr);
    // T must be a varbit type.
    void extract<T>(out T variableSizeHeader, in bit<32> sizeInBits);
    // does not advance the cursor
    T lookahead<T>();
    // skip this many bits from packet
    void advance(in bit<32> sizeInBits);
    // packet length in bytes
    bit<32> length();
}

extern packet_out {
    void emit<T>(in T hdr);
}

extern void verify(in bool check, in error toSignal);

action NoAction() {}

match_kind {
    exact,
    ternary,
    lpm
}

#endif  /* _CORE_P4_ */
