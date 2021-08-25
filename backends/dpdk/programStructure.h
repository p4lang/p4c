#ifndef BACKENDS_DPDK_PROGRAM_STRUCTURE_H_
#define BACKENDS_DPDK_PROGRAM_STRUCTURE_H_

#include "ir/ir.h"
#include "lib/cstring.h"

namespace DPDK {

class DpdkProgramStructure {
 public:
    // supported keys: ingress, egress
    ordered_map<cstring, const IR::Type_Header*> header_types;
    // supported keys: ingress, egress
    ordered_map<cstring, const IR::Type_Struct*> metadata_types;
    // supported keys: ingress, egress
    ordered_map<cstring, const IR::P4Parser*> parsers;
    // supported keys: ingress, egress
    ordered_map<cstring, const IR::P4Control*> deparsers;
    // supported keys: ingress, egress, pre_ingress /* pna */
    ordered_map<cstring, const IR::P4Control*> pipelines;

};

}  // namespace DPDK

#endif /* BACKENDS_DPDK_PROGRAM_STRUCTURE_H_ */
