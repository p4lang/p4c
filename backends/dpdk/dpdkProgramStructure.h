#ifndef BACKENDS_DPDK_PROGRAM_STRUCTURE_H_
#define BACKENDS_DPDK_PROGRAM_STRUCTURE_H_

#include "ir/ir.h"

/* Collect information related to P4 programs targeting dpdk */
class DpdkProgramStructure {
 public:
    ordered_map<cstring, const IR::Type_Header*> header_types;
    ordered_map<cstring, const IR::Type_Struct*> metadata_types;

    ordered_map<cstring, const IR::P4Parser*> parsers;
    ordered_map<cstring, const IR::P4Control*> deparsers;
    ordered_map<cstring, const IR::P4Control*> pipelines;

    std::map<const cstring, IR::IndexedVector<IR::Parameter> *> args_struct_map;
    std::map<const IR::Declaration_Instance *, cstring>         csum_map;
    std::map<cstring, int>                                      error_map;
    std::vector<const IR::Declaration_Instance *>               externDecls;

    IR::Type_Struct * metadataStruct;
};

#endif /* BACKENDS_DPDK_PROGRAM_STRUCTURE_H_ */
