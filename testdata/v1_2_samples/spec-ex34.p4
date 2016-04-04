extern Checksum16 
{
    // prepare unit
    void reset();
    // append data to be checksummed
    void append<D>(in D d); // same as { append(true, d); }
    // conditionally append data to be checksummed
    void append<D>(in bool condition, in D d);
    // get the checksum of all data appended since the last reset
    bit<16> get();
}
