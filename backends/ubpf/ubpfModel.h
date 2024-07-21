/*
Copyright 2019 Orange

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

#ifndef BACKENDS_UBPF_UBPFMODEL_H_
#define BACKENDS_UBPF_UBPFMODEL_H_

#include "frontends/common/model.h"
#include "frontends/p4/coreLibrary.h"
#include "ir/ir.h"
#include "ir/pass_manager.h"
#include "lib/cstring.h"

namespace p4c::UBPF {

using namespace ::p4c::P4::literals;

struct Pipeline_Model : public ::p4c::Model::Elem {
    Pipeline_Model()
        : Elem("Pipeline"_cs), parser("prs"_cs), control("p"_cs), deparser("dprs"_cs) {}

    ::p4c::Model::Elem parser;
    ::p4c::Model::Elem control;
    ::p4c::Model::Elem deparser;
};

struct Register_Model : public ::p4c::Model::Extern_Model {
    Register_Model()
        : Extern_Model("Register"_cs),
          sizeParam("size"_cs),
          read("read"_cs),
          write("write"_cs),
          initial_value("initial_value"_cs),
          index("index"_cs),
          value("value"_cs) {}

    ::p4c::Model::Elem sizeParam;
    ::p4c::Model::Elem read;
    ::p4c::Model::Elem write;
    ::p4c::Model::Elem initial_value;
    ::p4c::Model::Elem index;
    ::p4c::Model::Elem value;
};

struct Algorithm_Model : public ::p4c::Model::Enum_Model {
    Algorithm_Model() : ::p4c::Model::Enum_Model("HashAlgorithm"_cs), lookup3("lookup3"_cs) {}

    ::p4c::Model::Elem lookup3;
};

struct Hash_Model : public ::p4c::Model::Elem {
    Hash_Model() : ::p4c::Model::Elem("hash"_cs) {}
};

class UBPFModel : public ::p4c::Model::Model {
 protected:
    UBPFModel()
        : CPacketName("pkt"_cs),
          packet("packet"_cs, P4::P4CoreLibrary::instance().packetIn, 0),
          pipeline(),
          registerModel(),
          drop("mark_to_drop"_cs),
          pass("mark_to_pass"_cs),
          ubpf_time_get_ns("ubpf_time_get_ns"_cs),
          truncate("truncate"_cs),
          csum_replace2("csum_replace2"_cs),
          csum_replace4("csum_replace4"_cs),
          hashAlgorithm(),
          hash() {}

 public:
    static UBPFModel instance;
    static cstring reservedPrefix;

    ::p4c::Model::Elem CPacketName;
    ::p4c::Model::Param_Model packet;
    Pipeline_Model pipeline;
    Register_Model registerModel;
    ::p4c::Model::Elem drop;
    ::p4c::Model::Elem pass;
    ::p4c::Model::Elem ubpf_time_get_ns;
    ::p4c::Model::Elem truncate;
    ::p4c::Model::Extern_Model csum_replace2;
    ::p4c::Model::Extern_Model csum_replace4;
    Algorithm_Model hashAlgorithm;
    Hash_Model hash;
    unsigned version = 20200515;

    static cstring reserved(cstring name) { return reservedPrefix + name; }

    int numberOfParserArguments() const { return version >= 20200515 ? 4 : 3; }
    int numberOfControlBlockArguments() const { return version >= 20200515 ? 3 : 2; }

    class getUBPFModelVersion : public Inspector {
        bool preorder(const IR::Declaration_Constant *dc) override {
            if (dc->name == "__ubpf_model_version") {
                auto val = dc->initializer->to<IR::Constant>();
                UBPFModel::instance.version = static_cast<unsigned>(val->value);
            }
            return false;
        }
        bool preorder(const IR::Declaration *) override { return false; }
    };

    const IR::P4Program *run(const IR::P4Program *program) {
        if (program == nullptr) return nullptr;

        PassManager passes({
            new getUBPFModelVersion,
        });

        passes.setName("UBPFFrontEnd");
        passes.setStopOnError(true);
        const IR::P4Program *result = program->apply(passes);
        return result;
    }
};

}  // namespace p4c::UBPF

#endif /* BACKENDS_UBPF_UBPFMODEL_H_ */
