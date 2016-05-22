/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include "ir/ir.h"
#include "frontends/common/options.h"

IR::V1Program::V1Program(const CompilerOptions &) {
    // This should be kept in sync with v1model.p4
    auto fields = new IndexedVector<StructField>;
#define ADDF(name, type) do { \
    fields->push_back(new IR::StructField(IR::ID(name), type)); \
} while (0)

    ADDF("ingress_port", IR::Type::Bits::get(9));
    ADDF("packet_length", IR::Type::Bits::get(32));
    ADDF("egress_spec", IR::Type::Bits::get(9));
    ADDF("egress_port", IR::Type::Bits::get(9));
    ADDF("egress_instance", IR::Type::Bits::get(16));
    ADDF("instance_type", IR::Type::Bits::get(32));
    ADDF("parser_status", IR::Type::Bits::get(8));
    ADDF("parser_error_location", IR::Type::Bits::get(8));
#undef ADDF
    auto *standard_metadata_t = new IR::Type_Struct("standard_metadata_t", fields);

    scope.add("standard_metadata_t", new IR::v1HeaderType(standard_metadata_t));
    scope.add("standard_metadata", new IR::Metadata("standard_metadata", standard_metadata_t));
}
