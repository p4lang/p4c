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

#ifndef P4C_UBPFMODEL_H
#define P4C_UBPFMODEL_H

#include "frontends/common/model.h"
#include "frontends/p4/coreLibrary.h"
#include "ir/ir.h"
#include "lib/cstring.h"

namespace UBPF {

    struct Pipeline_Model : public ::Model::Elem {
        Pipeline_Model() : Elem("Pipeline"),
                         parser("prs"), control("p"), deparser("dprs") {}

        ::Model::Elem parser;
        ::Model::Elem control;
        ::Model::Elem deparser;
    };

    struct Register_Model : public ::Model::Extern_Model {
        Register_Model() : Extern_Model("Register"),
                           sizeParam("size"), read("read"), write("write"),
                           initial_value("initial_value"),
                           index("index"),
                           value("value") {}

        ::Model::Elem sizeParam;
        ::Model::Elem read;
        ::Model::Elem write;
        ::Model::Elem initial_value;
        ::Model::Elem index;
        ::Model::Elem value;
    };

    struct Algorithm_Model : public ::Model::Enum_Model {
        Algorithm_Model() : ::Model::Enum_Model("HashAlgorithm"),
                            lookup3("lookup3") {}

        ::Model::Elem lookup3;
    };

    struct Hash_Model : public ::Model::Elem {
        Hash_Model() : ::Model::Elem("hash") {}
    };

    class UBPFModel : public ::Model::Model {
    protected:
        UBPFModel() : Model("0.1"),
                      CPacketName("pkt"),
                      packet("packet", P4::P4CoreLibrary::instance.packetIn, 0),
                      pipeline(),
                      registerModel(),
                      drop("mark_to_drop"),
                      pass("mark_to_pass"),
                      ubpf_time_get_ns("ubpf_time_get_ns"),
                      csum_replace2("csum_replace2"),
                      csum_replace4("csum_replace4"),
                      hashAlgorithm(),
                      hash() {}

    public:
        static UBPFModel instance;
        static cstring reservedPrefix;

        ::Model::Elem CPacketName;
        ::Model::Param_Model packet;
        Pipeline_Model pipeline;
        Register_Model registerModel;
        ::Model::Elem drop;
        ::Model::Elem pass;
        ::Model::Elem ubpf_time_get_ns;
        ::Model::Extern_Model csum_replace2;
        ::Model::Extern_Model csum_replace4;
        Algorithm_Model hashAlgorithm;
        Hash_Model hash;

        static cstring reserved(cstring name) { return reservedPrefix + name; }
    };

}

#endif
