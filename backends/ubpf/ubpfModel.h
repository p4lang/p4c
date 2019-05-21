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

    struct TableImpl_Model : public ::Model::Extern_Model {
        explicit TableImpl_Model(cstring name) :
                Extern_Model(name),
                size("size") {}

        ::Model::Elem size;
    };

    struct Filter_Model : public ::Model::Elem {
        Filter_Model() : Elem("Filter"),
                         parser("prs"), filter("filt") {}

        ::Model::Elem parser;
        ::Model::Elem filter;
    };

    class UBPFModel : public ::Model::Model {
    protected:
        UBPFModel() : Model("0.1"),
                      hash_table("hash_table"),
                      tableImplProperty("implementation"),
                      CPacketName("pkt"),
                      packet("packet", P4::P4CoreLibrary::instance.packetIn, 0),
                      filter(), drop("mark_to_drop") {}

    public:
        static UBPFModel instance;
        static cstring reservedPrefix;

        TableImpl_Model hash_table;
        ::Model::Elem tableImplProperty;
        ::Model::Elem CPacketName;
        ::Model::Param_Model packet;
        Filter_Model filter;
        ::Model::Elem drop;

        static cstring reserved(cstring name) { return reservedPrefix + name; }
    };

}  // namespace EBPF

#endif /* P4C_UBPFMODEL_H */
