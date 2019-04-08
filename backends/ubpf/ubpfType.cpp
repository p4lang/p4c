#include "ubpfType.h"

namespace UBPF {

    EBPF::EBPFTypeFactory* instance = UBPFTypeFactory::getInstance();

    EBPF::EBPFType* UBPFTypeFactory::create(const IR::Type *type) {

        EBPF::EBPFType* result = nullptr;
        if (type->is<IR::Type_Boolean>()) {
            result = new EBPF::EBPFBoolType();
        } else if (auto bt = type->to<IR::Type_Bits>()) {
            result = new UBPFScalarType(bt); // using UBPF Scalar Type
        } else if (auto st = type->to<IR::Type_StructLike>()) {
            result = new EBPF::EBPFStructType(st);
        } else if (auto tt = type->to<IR::Type_Typedef>()) {
            auto canon = typeMap->getTypeType(type, true);
            result = create(canon);
            auto path = new IR::Path(tt->name);
            result = new EBPF::EBPFTypeName(new IR::Type_Name(path), result);
        } else if (auto tn = type->to<IR::Type_Name>()) {
            auto canon = typeMap->getTypeType(type, true);
            result = create(canon);
            result = new EBPF::EBPFTypeName(tn, result);
        } else if (auto te = type->to<IR::Type_Enum>()) {
            result = new EBPF::EBPFEnumType(te);
        } else if (auto ts = type->to<IR::Type_Stack>()) {
            auto et = create(ts->elementType);
            if (et == nullptr)
                return nullptr;
            result = new EBPF::EBPFStackType(ts, et);
        } else {
            ::error("Type %1% not supported", type);
        }
        return result;
    }

    //////////////////////////////////////////////////////////////////////////////

    void UBPFScalarType::emit(EBPF::CodeBuilder* builder) {
        // TODO: Note that it will handle only 8, 16, 32, or 64 width of header fields.
        //  Need to write method to parse custom width (e.g. 3 bits in MPLS header).
        if (width <= 8)
            builder->appendFormat("uint8_t");
        else if (width <= 16)
            builder->appendFormat("uint16_t");
        else if (width <= 32)
            builder->appendFormat("uint32_t");
        else if (width <= 64)
            builder->appendFormat("uint64_t");
        else
            builder->appendFormat("uint8_t*");

    }





}
