#include <core.p4>
#include <v1model.p4>

typedef bit<(48 + 12 + 9)> Mac_entry;
const bit<32> N_MAC_ENTRIES = 4096;
typedef register<Mac_entry> Mac_table;

control SnvsIngress(out Mac_entry b0) {
    Mac_table(N_MAC_ENTRIES) mac_table;

    apply {
	mac_table.read(b0, 0);
    }
}
