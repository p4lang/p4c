#include "backends/p4tools/common/lib/arch_spec.h"

#include <utility>

#include "lib/exceptions.h"

namespace P4Tools {

ArchSpec::ArchSpec(cstring packageName, const std::vector<ArchMember> &archVectorInput)
    : packageName(packageName) {
    for (size_t idx = 0; idx < archVectorInput.size(); ++idx) {
        auto archMember = archVectorInput.at(idx);
        archVector.emplace_back(archMember);
        auto blockName = archMember.blockName;
        blockIndices[blockName] = idx;
    }
}

size_t ArchSpec::getBlockIndex(cstring blockName) const {
    auto blockIndex = blockIndices.find(blockName);
    if (blockIndex != blockIndices.end()) {
        return blockIndex->second;
    }
    BUG("Unable to find block %s in the architecture map.", blockName);
}

cstring ArchSpec::getParamName(cstring blockName, size_t paramIndex) const {
    auto blockIndex = getBlockIndex(blockName);
    auto params = archVector.at(blockIndex).blockParams;
    BUG_CHECK(paramIndex < params.size(), "Param index %s out of range. Vector size: %s",
              paramIndex, params.size());
    return params.at(paramIndex);
}

cstring ArchSpec::getParamName(size_t blockIndex, size_t paramIndex) const {
    BUG_CHECK(blockIndex < archVector.size(), "Block index %s out of range. Vector size: %s",
              blockIndex, archVector.size());
    auto params = archVector.at(blockIndex).blockParams;
    BUG_CHECK(paramIndex < params.size(), "Param index %s out of range. Vector size: %s",
              paramIndex, params.size());
    return params.at(paramIndex);
}

const ArchSpec::ArchMember *ArchSpec::getArchMember(size_t blockIndex) const {
    BUG_CHECK(blockIndex < archVector.size(), "Index %s out of range. Vector size: %s", blockIndex,
              archVector.size());
    return &archVector.at(blockIndex);
}

size_t ArchSpec::getArchVectorSize() const { return archVector.size(); }

cstring ArchSpec::getPackageName() const { return packageName; }
}  // namespace P4Tools
