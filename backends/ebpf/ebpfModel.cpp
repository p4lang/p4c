#include "ebpfModel.h"

namespace EBPF {

cstring EBPFModel::reservedPrefix = "ebpf_";
EBPFModel EBPFModel::instance;

const IR::Type* EBPFModel::counterIndexType = IR::Type_Bits::get(32);
const IR::Type* EBPFModel::counterValueType = IR::Type_Bits::get(32);

}  // namespace EBPF
