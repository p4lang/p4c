#include "backends/p4tools/modules/testgen/targets/pna/concolic.h"

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
#include "backends/p4tools/common/lib/model.h"
#include "backends/p4tools/common/lib/util.h"
#include "ir/irutils.h"
#include "ir/vector.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

#include "backends/p4tools/modules/testgen/lib/concolic.h"
#include "backends/p4tools/modules/testgen/lib/exceptions.h"
#include "backends/p4tools/modules/testgen/lib/execution_state.h"

namespace P4Tools::P4Testgen::Pna {

const ConcolicMethodImpls::ImplList PnaDpdkConcolic::PNA_DPDK_CONCOLIC_METHOD_IMPLS{};

const ConcolicMethodImpls::ImplList *PnaDpdkConcolic::getPnaDpdkConcolicMethodImpls() {
    return &PNA_DPDK_CONCOLIC_METHOD_IMPLS;
}

}  // namespace P4Tools::P4Testgen::Pna
