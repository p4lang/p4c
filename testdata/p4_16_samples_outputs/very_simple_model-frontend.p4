#include <core.p4>

typedef bit<4> PortId;
struct InControl {
    PortId inputPort;
}

struct OutControl {
    PortId outputPort;
}

extern Checksum16 {
    void clear();
    void update<T>(in T data);
    bit<16> get();
}

