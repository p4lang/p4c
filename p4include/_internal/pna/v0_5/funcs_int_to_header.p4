/* The _int_to_header functions were written to convert a value of
 * type <name>_t (a value INTernal to the data path) to a value of
 * type <name>InHeader_t inside a header that will be sent to the CPU
 * port.
 *
 * The _header_to_int functions were written to convert values in the
 * opposite direction, typically for assigning a value in a header
 * received from the CPU port, to a value you wish to use in the rest
 * of your code.
 *
 * The reason that three casts are needed is that each of the original
 * and target types is declared via P4_16 'type', so without a cast
 * they can only be assigned to values of that identical type.  The
 * first cast changes it from the original 'type' to a 'bit<W1>' value
 * of the same bit width W1.  The second cast changes its bit width,
 * either prepending 0s if it becomes wider, or discarding the most
 * significant bits if it becomes narrower.  The third cast changes it
 * from a 'bit<W2>' value to the final 'type', with the same width
 * W2. */

PortId_t pna_PortId_header_to_int (in PortIdInHeader_t x) {
    return (PortId_t) (PortIdUint_t) (PortIdInHeaderUint_t) x;
}
InterfaceId_t pna_InterfaceId_header_to_int (in InterfaceIdInHeader_t x) {
    return (InterfaceId_t) (InterfaceIdUint_t) (InterfaceIdInHeaderUint_t) x;
}
MulticastGroup_t pna_MulticastGroup_header_to_int (in MulticastGroupInHeader_t x) {
    return (MulticastGroup_t) (MulticastGroupUint_t) (MulticastGroupInHeaderUint_t) x;
}
MirrorSessionId_t pna_MirrorSessionId_header_to_int (in MirrorSessionIdInHeader_t x) {
    return (MirrorSessionId_t) (MirrorSessionIdUint_t) (MirrorSessionIdInHeaderUint_t) x;
}
ClassOfService_t pna_ClassOfService_header_to_int (in ClassOfServiceInHeader_t x) {
    return (ClassOfService_t) (ClassOfServiceUint_t) (ClassOfServiceInHeaderUint_t) x;
}
PacketLength_t pna_PacketLength_header_to_int (in PacketLengthInHeader_t x) {
    return (PacketLength_t) (PacketLengthUint_t) (PacketLengthInHeaderUint_t) x;
}
MulticastInstance_t pna_MulticastInstance_header_to_int (in MulticastInstanceInHeader_t x) {
    return (MulticastInstance_t) (MulticastInstanceUint_t) (MulticastInstanceInHeaderUint_t) x;
}
Timestamp_t pna_Timestamp_header_to_int (in TimestampInHeader_t x) {
    return (Timestamp_t) (TimestampUint_t) (TimestampInHeaderUint_t) x;
}
FlowId_t pna_FlowId_header_to_int (in FlowIdInHeader_t x) {
    return (FlowId_t) (FlowIdUint_t) (FlowIdInHeaderUint_t) x;
}
ExpireTimeProfileId_t pna_ExpireTimeProfileId_header_to_int (in ExpireTimeProfileIdInHeader_t x) {
    return (ExpireTimeProfileId_t) (ExpireTimeProfileIdUint_t) (ExpireTimeProfileIdInHeaderUint_t) x;
}
PassNumber_t pna_PassNumber_header_to_int (in PassNumberInHeader_t x) {
    return (PassNumber_t) (PassNumberUint_t) (PassNumberInHeaderUint_t) x;
}

PortIdInHeader_t pna_PortId_int_to_header (in PortId_t x) {
    return (PortIdInHeader_t) (PortIdInHeaderUint_t) (PortIdUint_t) x;
}
InterfaceIdInHeader_t pna_InterfaceId_int_to_header (in InterfaceId_t x) {
    return (InterfaceIdInHeader_t) (InterfaceIdInHeaderUint_t) (InterfaceIdUint_t) x;
}
MulticastGroupInHeader_t pna_MulticastGroup_int_to_header (in MulticastGroup_t x) {
    return (MulticastGroupInHeader_t) (MulticastGroupInHeaderUint_t) (MulticastGroupUint_t) x;
}
MirrorSessionIdInHeader_t pna_MirrorSessionId_int_to_header (in MirrorSessionId_t x) {
    return (MirrorSessionIdInHeader_t) (MirrorSessionIdInHeaderUint_t) (MirrorSessionIdUint_t) x;
}
ClassOfServiceInHeader_t pna_ClassOfService_int_to_header (in ClassOfService_t x) {
    return (ClassOfServiceInHeader_t) (ClassOfServiceInHeaderUint_t) (ClassOfServiceUint_t) x;
}
PacketLengthInHeader_t pna_PacketLength_int_to_header (in PacketLength_t x) {
    return (PacketLengthInHeader_t) (PacketLengthInHeaderUint_t) (PacketLengthUint_t) x;
}
MulticastInstanceInHeader_t pna_MulticastInstance_int_to_header (in MulticastInstance_t x) {
    return (MulticastInstanceInHeader_t) (MulticastInstanceInHeaderUint_t) (MulticastInstanceUint_t) x;
}
TimestampInHeader_t pna_Timestamp_int_to_header (in Timestamp_t x) {
    return (TimestampInHeader_t) (TimestampInHeaderUint_t) (TimestampUint_t) x;
}
FlowIdInHeader_t pna_FlowId_int_to_header (in FlowId_t x) {
    return (FlowIdInHeader_t) (FlowIdInHeaderUint_t) (FlowIdUint_t) x;
}
ExpireTimeProfileIdInHeader_t pna_ExpireTimeProfileId_int_to_header (in ExpireTimeProfileId_t x) {
    return (ExpireTimeProfileIdInHeader_t) (ExpireTimeProfileIdInHeaderUint_t) (ExpireTimeProfileIdUint_t) x;
}
PassNumberInHeader_t pna_PassNumber_int_to_header (in PassNumber_t x) {
    return (PassNumberInHeader_t) (PassNumberInHeaderUint_t) (PassNumberUint_t) x;
}
