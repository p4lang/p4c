#include "lib/indent.h"
#include "blockMap.h"

namespace P4 {

const IR::PackageBlock* BlockMap::getMain() const {
    CHECK_NULL(program);

    auto main = program->getDeclByName(IR::P4Program::main);
    if (main == nullptr) {
        ::warning("Program does not contain a `%s' module", IR::P4Program::main);
        return nullptr;
    }
    if (!main->is<IR::Declaration_Instance>()) {
        ::error("%s must be a package declaration", main->getNode());
        return nullptr;
    }

    auto block = toplevelBlock->getValue(main->getNode());
    if (block == nullptr)
        return nullptr;
    return block->to<IR::PackageBlock>();
}

const IR::Block*
BlockMap::getBlockBoundToParameter(const IR::InstantiatedBlock* block, cstring paramName) const {
    auto result = block->getParameterValue(paramName);
    CHECK_NULL(result);
    BUG_CHECK(result->is<IR::Block>(), "%1% is not a block", result);
    return result->to<IR::Block>();
}


}  // namespace P4
