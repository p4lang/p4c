#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

typedef bit<69> Mac_entry;
const bit<32> N_MAC_ENTRIES = 32w4096;
typedef register<Mac_entry> Mac_table;
control SnvsIngress(out Mac_entry b0) {
    Mac_table(32w4096) mac_table;
    apply {
        mac_table.read(b0, 32w0);
    }
}

