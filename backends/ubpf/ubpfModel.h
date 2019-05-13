//
// Created by mateusz on 23.04.19.
//

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
        //TODO what exactly those below three parameters do?
        ::Model::Elem tableImplProperty;
        ::Model::Elem CPacketName;
        ::Model::Param_Model packet;
        Filter_Model filter;
        ::Model::Elem drop;

        static cstring reserved(cstring name) { return reservedPrefix + name; }
    };

}  // namespace EBPF

#endif /* P4C_UBPFMODEL_H */
