#include "ir/ir.h"
#include "frontends/common/options.h"

IR::V1Program::V1Program(const CompilerOptions &opt) {
    const IR::Type *port_t = IR::Type::Bits::get(9);
    if (opt.target == "tofino")
        port_t = IR::Type::Bits::get(16);
    // This should be kept in sync with v1model.p4
    auto fields = new NameMap<StructField, ordered_map>;
#define ADDF(name, type) do { \
    fields->addUnique(name, new IR::StructField(IR::ID(name), type)); \
} while (0)

    ADDF("ingress_port", port_t);
    ADDF("packet_length", IR::Type::Bits::get(32));
    ADDF("egress_spec", IR::Type::Bits::get(9));
    ADDF("egress_port", port_t);
    ADDF("egress_instance", IR::Type::Bits::get(16));
    ADDF("instance_type", IR::Type::Bits::get(32));
    ADDF("parser_status", IR::Type::Bits::get(8));
    ADDF("parser_error_location", IR::Type::Bits::get(8));
#undef ADDF
    auto *standard_metadata_t = new IR::Type_Struct("standard_metadata_t", std::move(*fields));

    scope.add("standard_metadata_t", new IR::v1HeaderType(standard_metadata_t));
    scope.add("standard_metadata", new IR::Metadata("standard_metadata", standard_metadata_t));
}
