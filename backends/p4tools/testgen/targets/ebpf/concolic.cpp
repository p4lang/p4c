#include "backends/p4tools/testgen/targets/ebpf/concolic.h"

#include <algorithm>
#include <iterator>
#include <map>
#include <utility>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/cpp_int/add.hpp>
#include <boost/multiprecision/cpp_int/import_export.hpp>
#include <boost/multiprecision/detail/et_ops.hpp>
#include <boost/multiprecision/number.hpp>
#include <boost/multiprecision/traits/explicit_conversion.hpp>

#include "backends/p4tools/common/lib/formulae.h"
#include "backends/p4tools/common/lib/ir.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

#include "backends/p4tools/testgen/lib/exceptions.h"
#include "backends/p4tools/testgen/lib/execution_state.h"

namespace P4Tools {

namespace P4Testgen {

namespace EBPF {

const ConcolicMethodImpls::ImplList EBPFConcolic::EBPFConcolicMethodImpls{};

const ConcolicMethodImpls::ImplList* EBPFConcolic::getEBPFConcolicMethodImpls() {
    return &EBPFConcolicMethodImpls;
}

}  // namespace EBPF

}  // namespace P4Testgen

}  // namespace P4Tools
