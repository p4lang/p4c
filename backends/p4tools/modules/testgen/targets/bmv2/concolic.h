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

class Bmv2Concolic : public Concolic {
 private:
    /// In the behavioral model, checksum functions have the following signature.
    using ChecksumFunction = std::function<big_int(const char *buf, size_t len)>;

    /// Chunk size is 8 bits, i.e., a byte.
    static constexpr int CHUNK_SIZE = 8;

    /// We are not using an enum class because we directly compare integers. This is because error
    /// types are converted into integers in our interpreter. If we use an enum class, we have to
    /// cast every enum access to int.
    struct Bmv2HashAlgorithm {
        using Type = enum {
            crc32,
            crc32_custom,
            crc16,
            crc16_custom,
            random,
            identity,
            csum16,
            xor16
        };
    };

    /// This is the list of concolic functions that are implemented in this class.
    static const ConcolicMethodImpls::ImplList BMV2_CONCOLIC_METHOD_IMPLS;

    /// Call into a behavioral model helper function to compute the appropriate checksum. The
    /// checksum is determined by @param algo.
    static big_int computeChecksum(const std::vector<const IR::Expression *> &exprList,
                                   const Model &finalModel, int algo,
                                   Model::ExpressionMap *resolvedExpressions);

    /// Compute a payload using the provided model and update the resolved concolic variables. Then
    /// return the computed payload expression. If the payload did not previously exist, a random
    /// variable according to @param payloadSize is generated. If the variable exists already, it
    /// just returns the payload expression.
    static const IR::Expression *setAndComputePayload(
        const Model &finalModel, ConcolicVariableMap *resolvedConcolicVariables, int payloadSize);

    /// Converts a big integer input into a vector of bytes. This byte vector is fed into the
    /// hash function.
    /// This function mimics the conversion of data structures to bytes in the behavioral model.
    static std::vector<char> convertBigIntToBytes(big_int &dataInt, int targetWidthBits);

 public:
    /// @returns the concolic  functions that are implemented for this particular target.
    static const ConcolicMethodImpls::ImplList *getBmv2ConcolicMethodImpls();
};

}  // namespace P4Tools::P4Testgen::Bmv2

#endif /* BACKENDS_P4TOOLS_MODULES_TESTGEN_TARGETS_BMV2_CONCOLIC_H_ */
