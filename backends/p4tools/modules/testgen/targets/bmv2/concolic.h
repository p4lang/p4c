#ifndef BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_CONCOLIC_H_
#define BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_CONCOLIC_H_

#include <cstddef>
#include <functional>
#include <vector>

#include "backends/p4tools/common/lib/model.h"
#include "ir/ir.h"
#include "lib/big_int_util.h"

#include "backends/p4tools/modules/testgen/lib/concolic.h"

namespace P4Tools::P4Testgen::Bmv2 {

enum class Bmv2HashAlgorithm {
    crc32,
    crc32_custom,
    crc16,
    crc16_custom,
    random,
    identity,
    csum16,
    xor16
};

std::ostream &operator<<(std::ostream &os, Bmv2HashAlgorithm algo);

class Bmv2Concolic : public Concolic {
 private:
    /// Chunk size is 8 bits, i.e., a byte.
    static constexpr int CHUNK_SIZE = 8;

    /// This is the list of concolic functions that are implemented in this class.
    static const ConcolicMethodImpls::ImplList BMV2_CONCOLIC_METHOD_IMPLS;

    /// Call into a behavioral model helper function to compute the appropriate checksum. The
    /// checksum is determined by @param algo.
    static big_int computeChecksum(const std::vector<const IR::Expression *> &exprList,
                                   const Model &finalModel, Bmv2HashAlgorithm algo,
                                   Model::ExpressionMap *resolvedExpressions);

    /// Compute a payload using the provided model and update the resolved concolic variables. Then
    /// return the computed payload expression. If the payload did not previously exist, a random
    /// variable according to @param payloadSize is generated. If the variable exists already, it
    /// just returns the payload expression.
    static const IR::Expression *setAndComputePayload(
        const Model &finalModel, ConcolicVariableMap *resolvedConcolicVariables, int payloadSize);

 public:
    /// @returns the concolic  functions that are implemented for this particular target.
    static const ConcolicMethodImpls::ImplList *getBmv2ConcolicMethodImpls();
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_CONCOLIC_H_ */
